/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 *
 *     http://www.apache.org/licenses/LICENSE-2.0 
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */

#ifndef MBM_NET_UTILS_H
#define MBM_NET_UTILS_H 1

int ifc_init(void);
void ifc_close(void);
int ifc_up(const char *name);
int ifc_down(const char *name);
int ifc_set_addr(const char *name, in_addr_t addr);
int ifc_set_mask(const char *name, in_addr_t mask);
int ifc_add_host_route(const char *name);
int ifc_configure(const char *ifname,
        in_addr_t address,
        in_addr_t gateway);

#endif
