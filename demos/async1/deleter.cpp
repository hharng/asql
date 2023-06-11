/*
 * SPDX-FileCopyrightText: (C) 2020 Daniel Nicoletti <dantti12@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "../../src/acache.h"
#include "../../src/adatabase.h"
#include "../../src/amigrations.h"
#include "../../src/apg.h"
#include "../../src/apool.h"
#include "../../src/aresult.h"
#include "../../src/atransaction.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QTimer>
#include <QUrl>
#include <QUuid>

using namespace ASql;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    APool::create(APg::factory(QStringLiteral("postgres:///?target_session_attrs=read-write")));
    APool::setSetupCallback([](ADatabase db) {
        qDebug() << "setup db";
        db.exec(u"SET TIME ZONE 'Europe/Rome';", nullptr, [](AResult &result) {
            qDebug() << "SETUP" << result.error() << result.errorString() << result.toJsonObject();
        });
    });

    APool::setReuseCallback([](ADatabase db) {
        qDebug() << "reuse db";
        db.exec(u"DISCARD ALL", nullptr, [](AResult &result) {
            qDebug() << "REUSE" << result.error() << result.errorString() << result.toJsonObject();
        });
    });

    auto obj = new QObject;
    {
        ADatabase db(APg::factory(QStringLiteral("postgres:///?target_session_attrs=read-write")));
        db.open(nullptr, [](bool ok, const QString &status) {
            qDebug() << "OPEN value" << ok << status;
        });
        db.exec(QStringLiteral("SELECT now()"), obj, [](AResult &result) {
            if (result.error()) {
                qDebug() << "SELECT error" << result.errorString();
                return;
            }

            if (result.size()) {
                qDebug() << "SELECT value" << result.begin().value(0);
            }
        });
    }

    APool::database().exec(u"SELECT pg_sleep(5)", obj, [](AResult &result) {
        qDebug() << "SELECT result.size()" << result.error() << result.errorString() << result.size();
    });

    APool::database().exec(u"SELECT now()", obj, [](AResult &result) {
        qDebug() << "SELECT result.size()" << result.error() << result.errorString() << result.toJsonObject();
    });

    QTimer::singleShot(2000, obj, [=] {
        qDebug() << "Delete Obj";
        delete obj;
    });

    QTimer::singleShot(2500, [=] {
        qDebug() << "Reuse Timer Obj";
        APool::database().exec(u"SELECT now()", nullptr, [](AResult &result) {
            qDebug() << "SELECT result.size()" << result.error() << result.errorString() << result.toJsonObject();
        });
    });

    app.exec();
}
