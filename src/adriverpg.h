/* 
 * SPDX-FileCopyrightText: (C) 2020 Daniel Nicoletti <dantti12@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef ADRIVERPG_H
#define ADRIVERPG_H

#include <adriver.h>
#include <libpq-fe.h>

#include "aresult.h"
#include "apreparedquery.h"

#include <queue>

#include <QPointer>
#include <QHash>

class QTimer;

namespace ASql {

class AResultPg final : public AResultPrivate
{
public:
    AResultPg();
    virtual ~AResultPg();

    bool lastResulSet() const override;
    bool error() const override;
    QString errorString() const override;

    QByteArray query() const override;

    int size() const override;
    int fields() const override;
    int numRowsAffected() const override;

    int indexOfField(QLatin1String name) const override;
    QString fieldName(int column) const override;
    QVariant value(int row, int column) const override;

    bool isNull(int row, int column) const override;
    bool toBool(int row, int column) const override;
    int toInt(int row, int column) const override;
    qint64 toLongLong(int row, int column) const override;
    quint64 toULongLong(int row, int column) const override;
    double toDouble(int row, int column) const override;
    QString toString(int row, int column) const override;
    std::string toStdString(int row, int column) const override;
    QDate toDate(int row, int column) const override;
    QTime toTime(int row, int column) const override;
    QDateTime toDateTime(int row, int column) const override;
    QJsonValue toJsonValue(int row, int column) const final;
    QByteArray toByteArray(int row, int column) const override;

    inline void processResult();

    QByteArray m_query;
    QString m_errorString;
    PGresult *m_result = nullptr;
    bool m_error = false;
    bool m_lastResultSet = true;
};

class APGQuery
{
public:
    APGQuery() : result(std::make_shared<AResultPg>())
    { }
    QByteArray query;
    APreparedQuery preparedQuery;
    std::shared_ptr<AResultPg> result;
    QVariantList params;
    AResultFn cb;
    QPointer<QObject> receiver;
    QObject *checkReceiver;
    bool preparing = false;
    bool prepared = false;
    bool setSingleRow = false;

    inline void done() {
        if (cb && (!checkReceiver || !receiver.isNull())) {
            result->m_query = query;
            AResult r(std::move(result));
            cb(r);
        }
    }
};

class ADriverPg final : public ADriver
{
    Q_OBJECT
public:
    ADriverPg(const QString &connInfo);
    virtual ~ADriverPg();

    bool isValid() const override;
    void open(std::function<void(bool isOpen, const QString &error)> cb) override;
    bool isOpen() const override;

    void setState(ADatabase::State state, const QString &status);
    ADatabase::State state() const override;
    void onStateChanged(std::function<void(ADatabase::State state, const QString &status)> cb) override;

    void begin(const std::shared_ptr<ADriver> &db, AResultFn cb, QObject *receiver) override;
    void commit(const std::shared_ptr<ADriver> &db, AResultFn cb, QObject *receiver) override;
    void rollback(const std::shared_ptr<ADriver> &db, AResultFn cb, QObject *receiver) override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void exec(const std::shared_ptr<ADriver> &db, QUtf8StringView query, const QVariantList &params, AResultFn cb, QObject *receiver) override;
#endif
    void exec(const std::shared_ptr<ADriver> &db, QStringView query, const QVariantList &params, AResultFn cb, QObject *receiver) override;
    void exec(const std::shared_ptr<ADriver> &db, const APreparedQuery &query, const QVariantList &params, AResultFn cb, QObject *receiver) override;

    void setLastQuerySingleRowMode() override;

    bool enterPipelineMode(qint64 autoSyncMS) override;

    bool exitPipelineMode() override;

    ADatabase::PipelineStatus pipelineStatus() const override;

    bool pipelineSync() override;

    void subscribeToNotification(const std::shared_ptr<ADriver> &db, const QString &name, ANotificationFn cb, QObject *receiver) override;
    QStringList subscribedToNotifications() const override;
    void unsubscribeFromNotification(const std::shared_ptr<ADriver> &db, const QString &name) override;

private:
    inline void queryConstructed(APGQuery &pgQuery);
    void nextQuery();
    void finishConnection();
    void finishQueries(const QString &error);
    inline void doExec(APGQuery &pgQuery);
    inline void doExecParams(APGQuery &query);
    inline void setSingleRowMode();
    inline void cmdFlush();

    std::function<void (ADatabase::State, const QString &)> m_stateChangedCb;
    QHash<QString, ANotificationFn> m_subscribedNotifications;
    std::queue<APGQuery> m_queuedQueries;
    std::shared_ptr<ADriver> selfDriver;
    QByteArrayList m_preparedQueries;
    QSocketNotifier *m_writeNotify = nullptr;
    QSocketNotifier *m_readNotify = nullptr;
    QTimer *m_autoSyncTimer = nullptr;
    PGconn *m_conn = nullptr;
    ADatabase::State m_state = ADatabase::State::Disconnected;
    int m_pipelineSync = 0;
    bool m_connected = false;
    bool m_flush = false;
    bool m_queryRunning = false;
    bool m_notificationPtrSet = false;
};

}

#endif // ADRIVERPG_H
