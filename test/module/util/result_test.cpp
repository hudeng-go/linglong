/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include "src/module/util/result.h"

using linglong::util::Result;

TEST(Moduel_Util, Result)
{
    auto funcLevel_1 = []() -> Result { return dResultBase() << -1 << "this is level 1 error"; };

    auto funcLevel_2 = [=]() -> Result {
        auto r = funcLevel_1();
        return dResult(r) << -2 << "this is level 2 error";
    };

    auto result = funcLevel_2();

    EXPECT_EQ(result.success(), false);
    EXPECT_EQ(result.code(), -2);

    qCritical() << result;
}
