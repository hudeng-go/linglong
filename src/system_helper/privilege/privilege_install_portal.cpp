/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "privilege_install_portal.h"

#include <QDir>
#include <QDirIterator>
#include <QDBusError>

#include "module/util/yaml.h"
#include "module/package/ref.h"

namespace linglong {
namespace system {
namespace helper {

const char *PrivilegePortalRule = R"MLS00(
whiteList:
  - org.deepin.screen-recorder
  - org.deepin.calendar
  - org.deepin.compressor
# TODO: use org.deepin.calendar instead
  - org.dde.calendar
# the target must be absolute path
fileRuleList:
  - source: files/lib/dde-dock/plugins/*.so
    target: /usr/lib/dde-dock/plugins
  - source: files/share/applications/context-menus/*.conf
    target: /usr/share/applications/context-menus
#  - source: files/share/glib-2.0/schemas/*.xml
#    target: /usr/share/glib-2.0/schemas
)MLS00";

static bool hasPrivilege(const QString &ref, const QStringList &whiteList)
{
    for (auto packageID : whiteList) {
        if (package::Ref(ref).appId == packageID) {
            return true;
        }
    }
    return false;
}

static void rebuildFileRule(const QString &installPath, const QString &ref, const FilePortalRule *rule)
{
    QDir sourceDir(installPath);

    qDebug() << "rebuild file rule" << installPath << ref << rule;
    QDirIterator iter(installPath, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        iter.next();
        auto relativeFilePath = sourceDir.relativeFilePath(iter.filePath());
        if (QDir::match(rule->source, relativeFilePath)) {
            auto targetFilePath = rule->target + QDir::separator() + iter.fileName();
            // TODO: save file to db and remove when uninstall
            QFileInfo targetFileInfo(targetFilePath);
            // FIXME: if another version is install, need an conflict or force, just remove is not so safe
            if (targetFileInfo.isSymLink()) {
                qWarning() << "remove" << targetFilePath << "-->" << QFile(targetFilePath).symLinkTarget();
                QFile::remove(targetFilePath);
            }
            qDebug() << "link file" << iter.filePath() << targetFilePath;
            if (!QFile::link(iter.filePath(), targetFilePath)) {
                qWarning() << "link failed";
            }
        }
    }
}

static void ruinFileRule(const QString &installPath, const QString &ref, const FilePortalRule *rule)
{
    QDir sourceDir(installPath);

    qDebug() << "ruin file rule" << installPath << ref << rule;
    QDirIterator iter(installPath, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        iter.next();
        auto relativeFilePath = sourceDir.relativeFilePath(iter.filePath());
        if (QDir::match(rule->source, relativeFilePath)) {
            auto targetFilePath = rule->target + QDir::separator() + iter.fileName();
            qDebug() << "remove link file" << iter.filePath() << targetFilePath;
            auto checkSource = QFile::symLinkTarget(targetFilePath);
            if (checkSource != iter.filePath()) {
                qWarning() << "ignore file not link to source version";
            } else {
                QFile::remove(targetFilePath);
            }
        }
    }
}

util::Error rebuildPrivilegeInstallPortal(const QString &installPath, const QString &ref, const QVariantMap &options)
{
    YAML::Node doc = YAML::Load(PrivilegePortalRule);
    QScopedPointer<PortalRule> privilegePortalRule(formYaml<PortalRule>(doc));

    if (!hasPrivilege(ref, privilegePortalRule->whiteList)) {
        return NewError(QDBusError::AccessDenied, "No Privilege Package");
    }

    for (auto rule : privilegePortalRule->fileRuleList) {
        if (!QDir::isAbsolutePath(rule->target)) {
            qWarning() << "target must be absolute path" << rule->target;
            continue;
        }
        rebuildFileRule(installPath, ref, rule);
    }

    return NoError();
}

util::Error ruinPrivilegeInstallPortal(const QString &installPath, const QString &ref, const QVariantMap &options)
{
    YAML::Node doc = YAML::Load(PrivilegePortalRule);
    QScopedPointer<PortalRule> privilegePortalRule(formYaml<PortalRule>(doc));

    if (!hasPrivilege(ref, privilegePortalRule->whiteList)) {
        return NewError(QDBusError::AccessDenied, "No Privilege Package");
    }

    for (auto rule : privilegePortalRule->fileRuleList) {
        if (!QDir::isAbsolutePath(rule->target)) {
            qWarning() << "target must be absolute path" << rule->target;
            continue;
        }
        ruinFileRule(installPath, ref, rule);
    }

    return NoError();
}

} // namespace helper
} // namespace system
} // namespace linglong
