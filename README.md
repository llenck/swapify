Swapify - Making things go brrr only when you need them
=======================================================

Swapify is a tool to stop processes and swap their memory onto disk. These programs can then later be swapped in again and will resume normal operation. This is done by injecting a shared object into the process that will spawn a second process living in the same memory space, which will, on request, send a SIGSTOP to the main process and write eligible mappings to `~/.swapify/[PID].swap`, which are then unmapped.

Motivation
----------

This software is useful for applications that consume a lot of memory, have a long startup time and are only used semi-regularly. For example, this could be used to host many minecraft servers on a single machine, which are swapped in using socket activation and a proxy.

Usage
-----

To use this, you have to spawn the process with `swapify <command>`. From then on you can swap the process by using `swap --action swap --pid <PID>` or `swap --action swap --socket <NAME>.<PID>.sock`. To unswap, just use `--action unswap` instead. Heres the output of `swap --help` for more info:
```
Usage: swap [options] [pids]
Where options are a combination of:
    -h, --help                print this help and exit
    -a, --action <ACTION>     action to send to specified pids and sockets (required)
    -p, --pid <PID>           act on PID (can be used multiple times)
    -s, --socket <NAME>       act on swapify instance listening at NAME
    -P, --socket-path <PATH>  path that sockets are resolved relative to

At least one PID or NAME is required. ACTION has to be one of swap, unswap
or exit. Only one ACTION may be specified. Additional arguments are
interpreted as additional pids to act on.

The default path that swap looks for sockets in is /run/user/[UID]/swapify,
which is also the default path used by swapify.
This program can only act on pids that have a swapify instance attached, and
it interacts with that instance by looking for sockets at PATH/PID.sock

This software is developed at https://github.com/llenck/swapify
```

It is _highly recommended_ to use a shell which is compatible with bash-like completion scripts, because `make install` puts one at `/etc/bash_completion.d/swap_completion.bash`, which makes using this software much easier by providing tab-completion to all of `swap`'s options, most importantly eligible PIDs and socket names.

Installing
----------

Just run `make release && sudo make install`. The default make target is a debug build, which will litter `~/.swapify/` with log files, so it is recommended to use a release build, unless you want to file a bug report.

Known Issues
------

There are currently quite a few issues with this software:
- Some commands seem to deadlock when used with `swapify` (known: steam, git push, minecraft-launcher).
- Programs use more memory than before after being unmapped, since swapify can't interact with the mechanism that Linux uses to only actually allocate pages when they're being used
- Swapify doesn't play nicely with 32-bit applications on 64-bit systems, but should work on systems that are all 32-bit.
- For now, this program will also kill the process on failure to unswap, but this might be fixed in the future.
- When used with certain programs, swapify creates many zombie processes, which could use up PIDs fairly quickly if you have them limited to a low value (check `sysctl kernel.pid_max`).

Lastly, I don't recommend using this Software for anything too important, as this is quite experimental and might crash a lot (though I personally don't know of any programs that crash unexpectedly). 

If you can figure out why some programs deadlock or find any application that segfaults when (un-)swapping, please create an issue at https://github.com/llenck/swapify/issues and include things like your processor architecture, kernel, C library implementation, Linux Distribution, log file (`~/.swapify/log-[SWAPIFY_PID]-[PID]-[random stuff]`) and the program that crashed in the bug report.
