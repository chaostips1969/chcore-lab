/*
 * Copyright (c) 2020 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * OS-Lab-2020 (i.e., ChCore) is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *   http://license.coscl.org.cn/MulanPSL
 *   THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 *   PURPOSE.
 *   See the Mulan PSL v1 for more details.
 */

#pragma once

#define EBUSY       -1 /* Busy */
#define EPERM       1 /* Operation not permitted */
#define EAGAIN      2 /* Try again */
#define ENOMEM      3 /* Out of memory */
#define EACCES      4 /* Permission denied */
#define EINVAL      5 /* Invalid argument */
#define EFBIG       6 /* File too large */
#define ENOSPC      7 /* No space left on device */
#define ENOSYS      8 /* Function not implemented */
#define ENODATA     9 /* No data available */
#define ETIME       10 /* Timer expired */
#define ECAPBILITY  11 /* Invalid capability */
#define ESUPPORT    12 /* Not supported */
#define EBADSYSCALL 13 /* Bad syscall number */
#define ENOMAPPING  14 /* Bad memory mapping */
#define ENOENT      15 /* Entry does not exist */
#define EEXIST      16 /* Entry already exists */
#define ENOTEMPTY   17 /* Dir is not empty */
#define ENOTDIR     18 /* Does not refer to a directory */
#define EFAULT      19 /* Bad address */

#define EMAX 20

#define ERR_PTR(x) ((void *)(s64)(x))
#define PTR_ERR(x) ((long)(x))
#define IS_ERR(x)  ((((s64)(x)) < 0) && ((s64)(x)) > -EMAX)
