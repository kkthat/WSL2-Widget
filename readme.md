
# WSL2 Widget

A very simple and probably not very good app that shows if WSL2 is running and allows you to stop it.


## Compiling

You can run this command to create a icon.res file that has the icon in it. (who'd have guessed)

```bash
  x86_64-w64-mingw32-windres icon.rc -O coff -o icon.res
```

Then you can run this to compile the exe. It should come out as a single exe with all the required libaries built in.

```bash
x86_64-w64-mingw32-g++ main.cpp icon.res -o WSLStatus.exe -mwindows -static-libgcc -static-libstdc++
```
## Installation

You can download a compiled exe from the releases page or you can build your own. If you want it to run on startup just put it in your windows startup folder. You can find it by running this command in WIN+R "shell:startup" Then just dump the exe in there.

