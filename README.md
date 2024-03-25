 <!-- **************************************************************************
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
 * Program/file : README.md                                                    *
 *                                                                             *
 * Description  : Top level readme file with general information and links     *
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
 *************************************************************************** -->
# MyDefence Device Interface (MDIF)
This repository contains the files needed to build a client interface to an MDIF
capable device. For information on what MDIF is refer to the
[documentation](https://mydefence.github.io/mdif/index.html). Documentation is
also found in the [docs](docs/) folder.

These files have been tested with UBUNTU 22.04 LTS operating system. Running on
a similar system is recommended.

Example client applications are provided:
* Linux/C client with serial connection to MDIF RFS device in
  [src/linux_rfs_demo](src/linux_rfs_demo/).
    * Application source demonstrates how to interact with a RF sensor
      (RFS) device.
    * Application source written in C and compiles to native executable.
    * Refer to [README.md](src/linux_rfs_demo/README.md) for more information
      and build instructions.
* Linux/C client with serial connection to MDIF RFE device in
  [src/linux_rfe_demo](src/linux_rfe_demo/).
    * Application source demonstrates how to interact with a RF emitter
      (RFE) device.
    * Application source written in C and compiles to native executable.
    * Refer to [README.md](src/linux_rfe_demo/README.md) for more information
      and build instructions.
* Android/Java client with serial connection to MDIF device in
  [src/android_demo](src/android_demo/).
    * Application source demonstrates how to interact with a RF sensor
      (RFS) device.
    * Application source written in Java and C and compiles to an APK.
    * Refer to [README.md](src/android_demo/README.md) for more information
      and build instructions.
* Node.js/Typescript client with network connection to MDIF device in
  [src/nodejs_demo](src/nodejs_demo/).
    * Application source demonstrates a bare minimum for device communication.
    * Application source written for Node.js in Typescript.
    * Refer to [README.md](src/nodejs_demo/README.md) for more information and
      build instructions.

MDIF messages are specified using [Protocol Buffers](https://protobuf.dev/), and
the specifications are found in the [src/protobuf](src/protobuf) folder. The
required .proto files needs to be compiled for the chosen implementation
language. The provided example applications demonstrate C and Typescript, but
any other language (like Python or Java) with a Protobuf compiler is useable.


