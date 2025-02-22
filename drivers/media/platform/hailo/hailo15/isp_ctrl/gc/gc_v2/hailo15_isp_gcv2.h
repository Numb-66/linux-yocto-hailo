/****************************************************************************
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2023 Vivante Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************
 *
 * The GPL License (GPL)
 *
 * Copyright (c) 2014-2023 Vivante Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 *****************************************************************************
 *
 * Note: This software is released under dual MIT and GPL licenses. A
 * recipient may use this file under the terms of either the MIT license or
 * GPL License. If you wish to use only one license not the other, you can
 * indicate your decision by deleting one of the above license notices in your
 * version of this file.
 *
 *****************************************************************************/

#ifndef __HAILO15_ISP_GC_V2_H__
#define __HAILO15_ISP_GC_V2_H__

#define HAILO15_ISP_CID_GC_ENABLE (HAILO15_ISP_CID_GC_BASE + 0x0000)
#define HAILO15_ISP_CID_GC_R_X_DISTANCE (HAILO15_ISP_CID_GC_BASE + 0x0001)
#define HAILO15_ISP_CID_GC_R_X_DATA (HAILO15_ISP_CID_GC_BASE + 0x0002)
#define HAILO15_ISP_CID_GC_R_Y_DATA (HAILO15_ISP_CID_GC_BASE + 0x0003)
#define HAILO15_ISP_CID_GC_G_X_DISTANCE (HAILO15_ISP_CID_GC_BASE + 0x0004)
#define HAILO15_ISP_CID_GC_G_X_DATA (HAILO15_ISP_CID_GC_BASE + 0x0005)
#define HAILO15_ISP_CID_GC_G_Y_DATA (HAILO15_ISP_CID_GC_BASE + 0x0006)
#define HAILO15_ISP_CID_GC_B_X_DISTANCE (HAILO15_ISP_CID_GC_BASE + 0x0007)
#define HAILO15_ISP_CID_GC_B_X_DATA (HAILO15_ISP_CID_GC_BASE + 0x0008)
#define HAILO15_ISP_CID_GC_B_Y_DATA (HAILO15_ISP_CID_GC_BASE + 0x0009)
#define HAILO15_ISP_CID_GC_MODE (HAILO15_ISP_CID_GC_BASE + 0x000A)

int hailo15_isp_gcv2_ctrl_count(void);
int hailo15_isp_gcv2_ctrl_create(struct hailo15_isp_device *isp_dev);

#endif