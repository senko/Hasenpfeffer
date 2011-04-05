Welcome to Hasenpfeffer!

This is a toy/educational microkernel OS built on top of L4 microkernel. It's actually a set of components running as regular processes on top of the micorkernel, and communicating using IPC.

An L4 primer
------------

In L4, the only thing that the kernel still does is ensuring memory protection, running user threads when needed, and providing IPC mechanisms. All the rest, even process and memory management, must be done by the userland.

Obviously, not all userland processes are allowed to do this. The initial "root-task" is special - L4 gives it the privileges to create, manipulate and destroy other tasks, threads and address spaces. In Hasenpfeffer, the Olymp server handles the roottask responsibilities.

The L4 IPC is a binary, register-based protocol. To avoid having to serialize/deserialize IPC messages manually, IDL4 (Interface Description Language for L4) is used. With IDL4, we can specify the service interface in a language similar to that of CORBA IDL, and get the generated stub files for client and server.

More info about L4:
 * http://www.l4hq.org/
 * http://www.l4ka.org/pistachio

Hasenpfeffer overview
---------------------

Not much information in English available yet. For the time being, the overview.txt has automatically-translated documentation from original Croatian sources.

Slides about Hasenpfeffer providing high-level overview:
 * http://www.slideshare.net/senkorasic/microkernelbased-operating-system-development
