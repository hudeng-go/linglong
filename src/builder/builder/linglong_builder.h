/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "builder.h"
#include "module/package/bundle.h"
#include "module/util/status_code.h"

namespace linglong {
namespace builder {

class LinglongBuilder
    : public QObject
    , public Builder
{
    Q_OBJECT
public:
    linglong::util::Error create(const QString &projectName) override;

    linglong::util::Error build() override;

    linglong::util::Error exportBundle(const QString &outputFilepath, bool useLocalDir) override;

    linglong::util::Error push(const QString &bundleFilePath, const QString &repoUrl, const QString &repoChannel, bool force) override;

    linglong::util::Error run() override;

    linglong::util::Error initRepo();
};

//TODO: remove later
class message : public JsonSerialize
{
    Q_OBJECT;
    Q_JSON_CONSTRUCTOR(message)
public:
    Q_JSON_PROPERTY(QString, type);
    Q_JSON_PROPERTY(int, pid);
    Q_JSON_PROPERTY(QString, arg0);
    Q_JSON_PROPERTY(int, wstatus);
    Q_JSON_PROPERTY(QString, information);
};
} // namespace builder
} // namespace linglong