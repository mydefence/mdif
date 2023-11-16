/*******************************************************************************
 *                                                                             *
 *                                                 ,,                          *
 *                                                       ,,,,,                 *
 *                                                           ,,,,,             *
 *           ,,,,,,,,,,,,,,,,,,,,,,,,,,,,                        ,,,,          *
 *          ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,            ,,,,          ,,,,        *
 *          ,,,,,       ,,,,,      ,,,,,,                ,,,,        ,,,       *
 *          ,,,,,       ,,,,,      ,,,,,,                   ,,,        ,,,     *
 *          ,,,,,       ,,,,,      ,,,,,,       ,,,           ,,,        ,     *
 *          ,,,,,       ,,,,,      ,,,,,,           ,,,         ,,        ,    *
 *          ,,,,,       ,,,,,      ,,,,,,              ,,        ,,            *
 *          ,,,,,       ,,,,,      ,,,,,,                ,        ,            *
 *          ,,,,,       ,,,,,      ,,,,,,                 ,                    *
 *          ,,,,,       ,,,,,      ,,,,,,                                      *
 *          ,,,,,       ,,,,,      ,,,,,,                                      *
 *                                       ,,,,,,,,,,,,,,,,,,,,,,,,,,            *
 *                                       ,,,,,,,,,,,,,,,,,,,,,,,,,,,,          *
 *                                       ,,,,,                  ,,,,,,         *
 *                     ,                 ,,,,,                  ,,,,,,         *
 *             ,        ,,               ,,,,,                  ,,,,,,         *
 *    ,        ,,        ,,,             ,,,,,                  ,,,,,,         *
 *     ,        ,,,         ,,,          ,,,,,                  ,,,,,,         *
 *     ,,,       ,,,                     ,,,,,                  ,,,,,,         *
 *      ,,,        ,,,,                  ,,,,,                  ,,,,,,         *
 *        ,,,         ,,,,               ,,,,,                  ,,,,,,         *
 *         ,,,,,            ,,,,         ,,,,,,,,,,,,,,,,,,,,,,,,,,,,          *
 *            ,,,,                       ,,,,,,,,,,,,,,,,,,,,,,,,,,            *
 *               ,,,,,                                                         *
 *                    ,,,,,                                                    *
 *                                                                             *
 * Program/file : mdif.ts                                                      *
 *                                                                             *
 * Description  :  Provides MDIF interface via a socket                        *
 *              :                                                              *
 *                                                                             *
 * Copyright 2023 MyDefence A/S.                                               *
 *                                                                             *
 * Licensed under the Apache License, Version 2.0 (the "License");             *
 * you may not use this file except in compliance with the License.            *
 * You may obtain a copy of the License at                                     *
 *                                                                             *
 * http://www.apache.org/licenses/LICENSE-2.0                                  *
 *                                                                             *
 * Unless required by applicable law or agreed to in writing, software         *
 * distributed under the License is distributed on an "AS IS" BASIS,           *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.    *
 * See the License for the specific language governing permissions and         *
 * limitations under the License.                                              *
 *                                                                             *
 *                                                                             *
 *                                                                             *
 *******************************************************************************/
import EventEmitter from 'events'
import type TypedEventEmitter from 'typed-emitter'
import net from 'net'
import { CoreMsg, GetDeviceInfoReq } from './_generated/mdif/core/core'

// {a:A} | {b:B, c:C} => 'a' | 'b' | 'c'
type KeysOfUnion<T> = T extends T ? keyof T : never
type KeysOfProtobuf<PB> = Exclude<KeysOfUnion<PB>, '$case'>
// PickType<{a:A} | {b:B, c:C}, 'c'>  => C
type PickType<T, K extends KeysOfUnion<T>> = T extends { [k in K]?: unknown } ? T[K] : never
// {a:A} | {b:B} => {a:A, b:B}
type Merge<T> = {
    [k in KeysOfProtobuf<T>]: (arg: PickType<T, k>) => void
}

export type CoreMap = Merge<CoreMsg['msg']>

/// Event emitter mapping from protobuf socket to json
export const mdif = new EventEmitter() as TypedEventEmitter<CoreMap>

function decodeMdifMsg(msg: Buffer): void {
    const coreMsg = CoreMsg.decode(msg)
    if (!coreMsg.msg) {
        console.log('not a core message!')
        return
    }
    console.log('coreMsg', coreMsg)

    const event = coreMsg.msg.$case
    // eslint-disable-next-line @typescript-eslint/ban-ts-comment
    // @ts-ignore
    const data = coreMsg.msg[event]
    if (!event || !data) {
        console.log('invalid message:', { coreMsg, event, data })
        return
    }
    mdif.emit(event, data)
}

export async function mdifService(ip: string, port: number): Promise<void> {
    const socket = new net.Socket()
    socket.on('error', (err) => {
        console.log('Socket error', err)
        process.exit(2)
    })

    mdif.on('getDeviceInfoReq', (req) => {
        const coreMsg = CoreMsg.create()
        coreMsg.msg = { $case: 'getDeviceInfoReq', getDeviceInfoReq: GetDeviceInfoReq.create(req) }
        const b = Buffer.from(CoreMsg.encode(coreMsg).finish())
        const l = Buffer.alloc(4)
        l.writeUint32LE(b.length)
        socket.write(Buffer.concat([l, b]))
    })

    let buf: Buffer | null = null
    socket.on('data', (data) => {
        if (buf) {
            data = Buffer.concat([buf, data])
            buf = null
        }

        let length = data.length
        while (length >= 4) {
            const n = data.readUint32LE()
            if (length < n + 4) {
                break
            }
            length -= n + 4
            decodeMdifMsg(data.subarray(4, 4 + n))
            data = data.subarray(4 + n)
        }
        if (length) {
            buf = data.subarray(-length)
        }
    })

    return new Promise<void>((resolve) => {
        socket.connect(port, ip, () => {
            console.log('Connected to device')
            resolve()
        })
    })
}
