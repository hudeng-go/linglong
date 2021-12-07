/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     huqinghong <huqinghong@uniontech.com>
 *
 * Maintainer: huqinghong <huqinghong@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "app_status.h"

// 安装数据库路径
const QString installedAppInfoPath = "/deepin/linglong/layers/";
// 安装数据库版本
const QString infoDbVersion = "1.0.0";

using namespace linglong::Status;

namespace linglong {
namespace util {

/*
 * 创建数据库连接
 *
 * @param dbConn: QSqlDatabase 数据库
 *
 * @return int: 0:创建成功 其它:失败
 */
int openDatabaseConnection(QSqlDatabase &dbConn)
{
    QString err = "";
    // 添加数据库驱动，并指定连接名称installed_package_connection
    if (QSqlDatabase::contains("installed_package_connection")) {
        dbConn = QSqlDatabase::database("installed_package_connection");
    } else {
        dbConn = QSqlDatabase::addDatabase("QSQLITE", "installed_package_connection");
        dbConn.setDatabaseName(installedAppInfoPath + "InstalledAppInfo.db");
    }
    if (!dbConn.isOpen() && !dbConn.open()) {
        err = "open " + installedAppInfoPath + "InstalledAppInfo.db failed";
        qCritical() << err;
        return StatusCode::FAIL;
    }
    return StatusCode::SUCCESS;
}

/*
 * 检查安装信息数据库表及版本信息表
 *
 * @return int: 0:成功 其它:失败
 */
int checkInstalledAppDb()
{
    QString err = "";
    QSqlDatabase dbConn;
    if (openDatabaseConnection(dbConn) != StatusCode::SUCCESS) {
        return StatusCode::FAIL;
    }
    QString createInfoTable = "CREATE TABLE IF NOT EXISTS installedAppInfo(\
         ID INTEGER PRIMARY KEY AUTOINCREMENT,\
         appId VARCHAR(32) NOT NULL,\
         name VARCHAR(32),\
         version VARCHAR(32) NOT NULL,\
         arch VARCHAR,\
         kind CHAR(8) DEFAULT 'app',\
         runtime CHAR(32),\
         uabUrl VARCHAR,\
         repoName CHAR(16),\
         description NVARCHAR,\
         user VARCHAR,\
         installType CHAR(16) DEFAULT 'user')";

    QSqlQuery sqlQuery(createInfoTable, dbConn);
    if (!sqlQuery.exec()) {
        err = "fail to create installed appinfo table, err:" + sqlQuery.lastError().text();
        qCritical() << err;
        dbConn.close();
        return StatusCode::FAIL;
    }

    QString createVersionTable = "CREATE TABLE IF NOT EXISTS appInfoDbVersion(\
         version VARCHAR(32) PRIMARY KEY,description NVARCHAR)";
    sqlQuery.prepare(createVersionTable);
    if (!sqlQuery.exec()) {
        err = "fail to create appInfoDbVersion, err:" + sqlQuery.lastError().text();
        qCritical() << err;
        dbConn.close();
        return StatusCode::FAIL;
    }
    dbConn.close();
    return StatusCode::SUCCESS;
}

/*
 * 更新安装数据库版本
 *
 * @return int: 0:成功 其它:失败
 */
int updateInstalledAppInfoDb()
{
    QString err = "";
    QSqlDatabase dbConn;
    if (openDatabaseConnection(dbConn) != StatusCode::SUCCESS) {
        return StatusCode::FAIL;
    }

    // 版本升序排列
    QString selectSql = "SELECT * FROM appInfoDbVersion order by version ASC ";
    QSqlQuery sqlQuery(selectSql, dbConn);
    if (!sqlQuery.exec()) {
        err = "checkAppDbUpgrade fail to exec sql:" + selectSql + ", error:" + sqlQuery.lastError().text();
        qCritical() << err;
        dbConn.close();
        return StatusCode::FAIL;
    }

    // sqlite3不支持size属性
    sqlQuery.last();
    int recordCount = sqlQuery.at() + 1;
    // 首次安装
    QString description = "create appinfodb version";
    QString insertSql = "";
    if (recordCount < 1) {
        insertSql = QString("INSERT INTO appInfoDbVersion VALUES('%1','%2')").arg(infoDbVersion).arg(description);
    } else {
        // fix to do update version
        QString currentVersion = sqlQuery.value(0).toString().trimmed();
        if (currentVersion < infoDbVersion) {
            insertSql = QString("INSERT INTO appInfoDbVersion VALUES('%1','%2')").arg(infoDbVersion).arg(description);
        }
        qDebug() << "installedAppInfoDb currentVersion:" << currentVersion << ", dstVersion:" << infoDbVersion;
    }
    if (!insertSql.isEmpty()) {
        sqlQuery.prepare(insertSql);
        if (!sqlQuery.exec()) {
            err = "checkAppDbUpgrade fail to exec sql:" + insertSql + ", error:" + sqlQuery.lastError().text();
            qCritical() << err;
            dbConn.close();
            return StatusCode::FAIL;
        }
    }
    dbConn.close();
    return StatusCode::SUCCESS;
}

/*
 * 增加软件包安装信息
 *
 * @param package: 软件包信息
 * @param installType: 软件包安装类型
 * @param userName: 用户名
 *
 * @return int: 0:成功 其它:失败
 */
int insertAppRecord(AppStreamPkgInfo package, const QString &installType, const QString &userName)
{
    QString err = "";
    QSqlDatabase dbConn;
    if (openDatabaseConnection(dbConn) != StatusCode::SUCCESS) {
        return StatusCode::FAIL;
    }

    QSqlQuery sqlQuery(dbConn);
    // installType 字段暂时保留
    QString insertSql =
        QString("INSERT INTO installedAppInfo(appId,name,version,arch,kind,runtime,uabUrl,repoName,description,user)\
    VALUES('%1','%2','%3','%4','%5','%6','%7','%8','%9','%10')")
            .arg(package.appId)
            .arg(package.name)
            .arg(package.version)
            //.arg(package.appArch.join(","))
            .arg(package.arch)
            .arg(package.kind)
            .arg(package.runtime)
            .arg(package.uabUrl)
            .arg(package.repoName)
            .arg(package.description)
            .arg(userName);

    sqlQuery.prepare(insertSql);
    if (!sqlQuery.exec()) {
        err = "insertAppRecord fail to exec sql:" + insertSql + ", error:" + sqlQuery.lastError().text();
        qCritical() << err;
        dbConn.close();
        return StatusCode::FAIL;
    }
    dbConn.close();
    qDebug() << "insertAppRecord app:" << package.appId << ", version:" << package.version << " success";
    return StatusCode::SUCCESS;
}

/*
 * 删除软件包安装信息
 *
 * @param appId: 软件包包名
 * @param appVer: 软件包对应的版本号
 * @param appArch: 软件包对应的架构
 * @param userName: 用户名
 *
 * @return int: 0:成功 其它:失败
 */
int deleteAppRecord(const QString &appId, const QString &appVer, const QString &appArch, const QString &userName)
{
    QString err = "";
    // 查询是否安装由调用方负责
    QSqlDatabase dbConn;
    if (openDatabaseConnection(dbConn) != StatusCode::SUCCESS) {
        return StatusCode::FAIL;
    }

    // 若未指定版本，则查找最高版本
    QString dstVer = appVer;
    QSqlQuery sqlQuery(dbConn);
    if (appVer.isEmpty()) {
        // 查找最高版本
        QString selectSql =
            QString("SELECT version FROM installedAppInfo WHERE appId = '%1' AND user = '%2' order by version DESC")
                .arg(appId)
                .arg(userName);
        sqlQuery.prepare(selectSql);
        if (!sqlQuery.exec()) {
            err = "deleteAppRecord fail to exec sql:" + selectSql + ", error:" + sqlQuery.lastError().text();
            qCritical() << err;
            dbConn.close();
            return StatusCode::FAIL;
        }

        // sqlite3不支持size属性
        sqlQuery.last();
        int recordCount = sqlQuery.at() + 1;
        if (recordCount < 1) {
            err = "deleteAppRecord app:" + appId + ",userName:" + userName + " not found";
            qCritical() << err;
            dbConn.close();
            return StatusCode::FAIL;
        }

        // int fieldNo = query.record().indexOf("version");
        sqlQuery.first();
        dstVer = sqlQuery.value(0).toString().trimmed();
    }

    QString deleteSql = QString("DELETE FROM installedAppInfo WHERE appId = '%1' AND version = '%2' AND user = '%3'")
                            .arg(appId)
                            .arg(dstVer)
                            .arg(userName);
    sqlQuery.prepare(deleteSql);
    if (!sqlQuery.exec()) {
        err = "deleteAppRecord fail to exec sql:" + deleteSql + ", error:" + sqlQuery.lastError().text();
        qCritical() << err;
        dbConn.close();
        return StatusCode::FAIL;
    }
    dbConn.close();
    qDebug() << "delete app:" << appId << ", version:" << dstVer << ", arch:" << appArch << " success";
    return StatusCode::SUCCESS;
}

/*
 * 查询软件包是否安装
 *
 * @param appId: 软件包包名
 * @param appVer: 软件包对应的版本号
 * @param appArch: 软件包对应的架构
 * @param userName: 用户名
 *
 * @return bool: true:已安装 false:未安装
 */
bool getAppInstalledStatus(const QString &appId, const QString &appVer, const QString &appArch, const QString &userName)
{
    QString err = "";
    QSqlDatabase dbConn;
    if (openDatabaseConnection(dbConn) != StatusCode::SUCCESS) {
        return false;
    }
    QSqlQuery sqlQuery(dbConn);

    // 默认不查找版本 fix to runtime runtime应该不区分用户
    QString selectSql =
        QString("SELECT * FROM installedAppInfo WHERE appId = '%1' AND user = '%2'").arg(appId).arg(userName);

    if (!appVer.isEmpty()) {
        selectSql = QString("SELECT * FROM installedAppInfo WHERE appId = '%1' AND version = '%2' AND user = '%3' ")
                        .arg(appId)
                        .arg(appVer)
                        .arg(userName);
    }

    // 查找指定版本和架构 指定架构必须指定版本 暂不支持单独指定架构
    if (!appArch.isEmpty() && !appVer.isEmpty()) {
        selectSql =
            QString(
                "SELECT * FROM installedAppInfo WHERE appId = '%1' AND version = '%2' AND user = '%3' AND arch like '%%4%'")
                .arg(appId)
                .arg(appVer)
                .arg(userName)
                .arg(appArch);
    }
    sqlQuery.prepare(selectSql);
    if (!sqlQuery.exec()) {
        err = "getAppInstalledStatus fail to exec sql:" + selectSql + ", error:" + sqlQuery.lastError().text();
        qCritical() << err;
        dbConn.close();
        return false;
    }
    // 指定用户和版本找不到
    // sqlite3不支持size属性
    sqlQuery.last();
    int recordCount = sqlQuery.at() + 1;
    if (recordCount < 1) {
        err = "getAppInstalledStatus app:" + appId + ",version:" + appVer + ",userName:" + userName + " not found";
        qDebug() << err;
        dbConn.close();
        return false;
    }
    dbConn.close();
    return true;
}

/*
 * 查询已安装软件包信息
 *
 * @param appId: 软件包包名
 * @param appVer: 软件包对应的版本号
 * @param appArch: 软件包对应的架构
 * @param userName: 用户名
 * @param pkgList: 查询结果
 *
 * @return bool: true:成功 false:失败
 */
bool getInstalledAppInfo(const QString &appId, const QString &appVer, const QString &appArch, const QString &userName,
                         PKGInfoList &pkgList)
{
    QString err = "";
    if (!getAppInstalledStatus(appId, appVer, appArch, userName)) {
        err = "getInstalledAppInfo app:" + appId + ",version:" + appVer + ",userName:" + userName + " not installed";
        qCritical() << err;
        return false;
    }

    QSqlDatabase dbConn;
    if (openDatabaseConnection(dbConn) != StatusCode::SUCCESS) {
        return false;
    }

    QSqlQuery sqlQuery(dbConn);
    QString selectSql =
        QString("SELECT * FROM installedAppInfo WHERE appId = '%1' AND user = '%2' order by version ASC")
            .arg(appId)
            .arg(userName);
    if (!appVer.isEmpty()) {
        selectSql = QString("SELECT * FROM installedAppInfo WHERE appId = '%1' AND version = '%2' AND user = '%3'")
                        .arg(appId)
                        .arg(appVer)
                        .arg(userName);
    }
    sqlQuery.prepare(selectSql);
    if (!sqlQuery.exec()) {
        err = "getInstalledAppInfo fail to exec sql:" + selectSql + ", error:" + sqlQuery.lastError().text();
        qCritical() << err;
        dbConn.close();
        return false;
    }

    // 多个版本返回最高版本信息
    sqlQuery.last();
    int recordCount = sqlQuery.at() + 1;
    if (recordCount > 0) {
        auto info = QPointer<PKGInfo>(new PKGInfo);
        info->appid = sqlQuery.value(1).toString().trimmed();
        info->appname = sqlQuery.value(2).toString().trimmed();
        info->version = sqlQuery.value(3).toString().trimmed();
        info->arch = sqlQuery.value(4).toString().trimmed();
        info->description = sqlQuery.value(9).toString().trimmed();
        pkgList.push_back(info);
        return true;
    }
    return false;
}

/*
 * 查询所有已安装软件包信息
 *
 * @param userName: 用户名
 *
 * @return pkgList:查询结果
 */
PKGInfoList queryAllInstalledApp(const QString &userName)
{
    QString err = "";
    QSqlDatabase dbConn;
    if (openDatabaseConnection(dbConn) != StatusCode::SUCCESS) {
        return {};
    }
    QSqlQuery sqlQuery(dbConn);

    PKGInfoList pkglist;
    // 默认不查找版本
    QString selectSql = "";
    if (userName.isEmpty()) {
        selectSql = QString("SELECT * FROM installedAppInfo");
    } else {
        selectSql = QString("SELECT * FROM installedAppInfo WHERE user = '%1'").arg(userName);
    }
    sqlQuery.prepare(selectSql);
    if (!sqlQuery.exec()) {
        err = "queryAllInstalledApp fail to exec sql:" + selectSql + ", error:" + sqlQuery.lastError().text();
        qCritical() << err;
        dbConn.close();
        return {};
    }
    while (sqlQuery.next()) {
        auto info = QPointer<PKGInfo>(new PKGInfo);
        info->appid = sqlQuery.value(1).toString().trimmed();
        info->appname = sqlQuery.value(2).toString().trimmed();
        info->version = sqlQuery.value(3).toString().trimmed();
        info->arch = sqlQuery.value(4).toString().trimmed();
        info->description = sqlQuery.value(9).toString().trimmed();
        pkglist.push_back(info);
    }
    return pkglist;
}
} // namespace util
} // namespace linglong