/*
 * Copyright (c) 2022. ${ORGANIZATION_NAME}. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QDir>
#include "project.h"

#include "module/repo/ostree_repo.h"

#include "builder_config.h"
#include "depend_fetcher.h"
#include "project.h"

namespace linglong {
namespace builder {

class DependFetcherPrivate
{
public:
    explicit DependFetcherPrivate(const BuildDepend &bd, Project *parent)
        : ref(fuzzyRef(&bd)), project(parent), dependType(bd.type)
    {
    }
    //TODO: dependType should be removed, buildDepend include it
    package::Ref ref;
    Project *project;
    QString dependType;
};

DependFetcher::DependFetcher(const BuildDepend &bd, Project *parent)
    : QObject(parent)
    , dd_ptr(new DependFetcherPrivate(bd, parent))
{
}

DependFetcher::~DependFetcher() = default;

linglong::util::Error DependFetcher::fetch(const QString &subPath, const QString &targetPath)
{
    repo::OSTreeRepo ostree(BuilderConfig::instance()->repoPath());

    auto remoteRef = package::Ref("", dd_ptr->ref.appId, dd_ptr->ref.version, dd_ptr->ref.arch);

    if (!ostree.isRefExists(remoteRef)) {
        // TODO: support more channel
        remoteRef = package::Ref("", "linglong", dd_ptr->ref.appId, dd_ptr->ref.version, dd_ptr->ref.arch);
        auto ret = ostree.pullAll(remoteRef, true);
        if (!ret.success()) {
            return NewError(ret, -1, "pull " + remoteRef.toString() + " failed");
        }
    }
    
    QDir targetParentDir(targetPath);
    targetParentDir.cdUp();
    targetParentDir.mkpath(".");

    auto ret = ostree.checkoutAll(remoteRef, subPath, targetPath);

    //for app,lib. if the dependType match runtime, should be submitted together.
    if (dd_ptr->dependType == DependTypeRuntime) {
        auto targetInstallPath = dd_ptr->project->config().cacheAbsoluteFilePath(
            {"overlayfs", "up", dd_ptr->project->config().targetInstallPath("")});

        ret = ostree.checkoutAll(remoteRef, subPath, targetInstallPath);
    }

    return WrapError(ret, QString("ostree checkout %1 with subpath '%2' to %3")
                              .arg(remoteRef.toLocalRefString())
                              .arg(subPath)
                              .arg(targetPath));

}

} // namespace builder
} // namespace linglong
