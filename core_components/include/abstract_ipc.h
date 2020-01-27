/*
*   Copyright 2020 Check Point Software Technologies LTD
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*       you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*/

#ifndef __ABSTRACT_IPC_H__
#define __ABSTRACT_IPC_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef struct AbstractIpc AbstractIpc;

typedef struct ReceivedData
{
    void *internal_reference;
    const u_char *buffer;
    uint size;
} ReceivedData;

AbstractIpc * initIpc(int is_first_side);

void finiIpc(AbstractIpc *ipc);

int sendData(AbstractIpc *ipc, uint buffer_size, const u_char *buffer);

int sendChunkedData(
    AbstractIpc *ipc,
    uint16_t *buffer_sizes,
    u_char **buffers,
    uint num_of_buffers);

int receiveData(AbstractIpc *ipc, ReceivedData *data);

int releaseData(ReceivedData *data);

int isDataAvailable(AbstractIpc *ipc);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __ABSTRACT_IPC_H__
