# 源码构建说明

本文说明如何从源码构建当前 `CommAssistant-SC-Base` 工程。工程使用 Qt qmake，不使用 CMake。

## 1. 环境要求

推荐环境：

- Windows 10/11
- Qt 6.10.3，组件选择 `MinGW 13.1.0 64-bit`
- PowerShell
- Git

当前已验证可用的 Qt 路径示例：

- Qt 库：`D:/Tools/Qt/6.10.3/mingw_64`
- Qt 自带 MinGW：`D:/Tools/Qt/Tools/mingw1310_64`

如果你的 Qt 安装目录不同，后续命令中的路径需要替换为自己的实际路径。

## 2. 关键注意事项

必须使用 Qt 安装器自带的 MinGW 工具链构建，不能混用其他 MinGW。

例如，如果系统 PATH 中优先出现了 WinLibs/GCC 15，链接阶段可能报错：

```text
undefined reference to `__imp___argc'
```

解决方式是在当前 PowerShell 会话中把 Qt MinGW 和 Qt bin 放到 PATH 最前面：

```powershell
$env:PATH="D:/Tools/Qt/Tools/mingw1310_64/bin;D:/Tools/Qt/6.10.3/mingw_64/bin;" + $env:PATH
```

## 3. 首次构建 Release

从仓库根目录执行：

```powershell
cd "E:/projects/projects/8y/CommAssistant-SC-Base"

New-Item -ItemType Directory -Force "build-mingw" | Out-Null
cd "build-mingw"

$env:PATH="D:/Tools/Qt/Tools/mingw1310_64/bin;D:/Tools/Qt/6.10.3/mingw_64/bin;" + $env:PATH

& "D:/Tools/Qt/6.10.3/mingw_64/bin/qmake.exe" "../ScriptCommunicator/ScriptCommunicator.pro" -spec win32-g++ "CONFIG+=release"
& "D:/Tools/Qt/Tools/mingw1310_64/bin/mingw32-make.exe" -j8 release
```

构建成功后，主程序位于：

```text
CommAssistant-SC-Base/build-mingw/release/ScriptCommunicator.exe
```

## 4. 增量构建

如果已经生成过 `build-mingw/Makefile`，后续改代码后通常只需要：

```powershell
cd "E:/projects/projects/8y/CommAssistant-SC-Base/build-mingw"

$env:PATH="D:/Tools/Qt/Tools/mingw1310_64/bin;D:/Tools/Qt/6.10.3/mingw_64/bin;" + $env:PATH

& "D:/Tools/Qt/Tools/mingw1310_64/bin/mingw32-make.exe" -j8 release
```

如果改了 `.pro` 文件或新增/删除源码文件，建议重新运行一次 `qmake`，再 `make`。

## 5. 运行程序

开发机上可以直接运行：

```powershell
cd "E:/projects/projects/8y/CommAssistant-SC-Base/build-mingw/release"
.\ScriptCommunicator.exe
```

如果提示缺少 Qt DLL，说明运行目录没有部署 Qt 运行库。执行：

```powershell
cd "E:/projects/projects/8y/CommAssistant-SC-Base/build-mingw"

$env:PATH="D:/Tools/Qt/Tools/mingw1310_64/bin;D:/Tools/Qt/6.10.3/mingw_64/bin;" + $env:PATH

& "D:/Tools/Qt/6.10.3/mingw_64/bin/windeployqt.exe" --release "release/ScriptCommunicator.exe"
```

部署完成后，再运行：

```powershell
cd "E:/projects/projects/8y/CommAssistant-SC-Base/build-mingw/release"
.\ScriptCommunicator.exe
```

## 6. Debug 构建

如需 Debug 版本：

```powershell
cd "E:/projects/projects/8y/CommAssistant-SC-Base"

New-Item -ItemType Directory -Force "build-mingw" | Out-Null
cd "build-mingw"

$env:PATH="D:/Tools/Qt/Tools/mingw1310_64/bin;D:/Tools/Qt/6.10.3/mingw_64/bin;" + $env:PATH

& "D:/Tools/Qt/6.10.3/mingw_64/bin/qmake.exe" "../ScriptCommunicator/ScriptCommunicator.pro" -spec win32-g++ "CONFIG+=debug"
& "D:/Tools/Qt/Tools/mingw1310_64/bin/mingw32-make.exe" -j8 debug
```

Debug 程序位于：

```text
CommAssistant-SC-Base/build-mingw/debug/ScriptCommunicator.exe
```

## 7. 常见问题

### qmake 找不到

不要依赖系统 PATH，直接使用 Qt 的完整路径：

```powershell
& "D:/Tools/Qt/6.10.3/mingw_64/bin/qmake.exe" "../ScriptCommunicator/ScriptCommunicator.pro" -spec win32-g++ "CONFIG+=release"
```

### mingw32-make 使用了错误的 g++

先检查：

```powershell
Get-Command g++
Get-Command mingw32-make
```

推荐在构建前强制设置 PATH：

```powershell
$env:PATH="D:/Tools/Qt/Tools/mingw1310_64/bin;D:/Tools/Qt/6.10.3/mingw_64/bin;" + $env:PATH
```

### 链接阶段出现 `undefined reference to __imp___argc`

这是典型的 MinGW 工具链混用问题。确认 `g++` 来自：

```text
D:/Tools/Qt/Tools/mingw1310_64/bin/g++.exe
```

然后重新执行构建命令。

### 运行时缺少 Qt DLL

执行 `windeployqt`：

```powershell
& "D:/Tools/Qt/6.10.3/mingw_64/bin/windeployqt.exe" --release "release/ScriptCommunicator.exe"
```

### 修改 UI 后没有生效

如果修改了 `.ui` 文件，qmake/make 会调用 `uic` 重新生成 `ui_*.h`。如果怀疑缓存问题，重新运行：

```powershell
& "D:/Tools/Qt/6.10.3/mingw_64/bin/qmake.exe" "../ScriptCommunicator/ScriptCommunicator.pro" -spec win32-g++ "CONFIG+=release"
& "D:/Tools/Qt/Tools/mingw1310_64/bin/mingw32-make.exe" -j8 release
```

## 8. 推荐给新接手者的最短命令

如果 Qt 安装路径与本文一致，直接执行：

```powershell
cd "E:/projects/projects/8y/CommAssistant-SC-Base"
New-Item -ItemType Directory -Force "build-mingw" | Out-Null
cd "build-mingw"
$env:PATH="D:/Tools/Qt/Tools/mingw1310_64/bin;D:/Tools/Qt/6.10.3/mingw_64/bin;" + $env:PATH
& "D:/Tools/Qt/6.10.3/mingw_64/bin/qmake.exe" "../ScriptCommunicator/ScriptCommunicator.pro" -spec win32-g++ "CONFIG+=release"
& "D:/Tools/Qt/Tools/mingw1310_64/bin/mingw32-make.exe" -j8 release
& "D:/Tools/Qt/6.10.3/mingw_64/bin/windeployqt.exe" --release "release/ScriptCommunicator.exe"
```

## 9. 只交付一个 exe 的打包方式

当前环境使用的是动态 Qt，不是静态 Qt，因此不能仅靠改工程文件就得到真正“静态链接”的单文件 exe。

如果目标是“最终交付时只给用户一个 exe”，可以使用仓库里的打包脚本：

```powershell
cd "E:/projects/projects/8y/CommAssistant-SC-Base"
.\scripts\package_single_exe.ps1
```

这个脚本会：

- 先生成/更新 `release` 版程序
- 使用 `windeployqt` 收集 Qt 运行库
- 把部署目录打成 zip 并嵌入到一个启动器 exe 中
- 用户最终只需要拿到一个 exe 文件

注意：

- 这不是静态 Qt 链接。
- 这是“单文件自解压启动包”。
- 如果你必须要“真正不带任何外部依赖的原生单 exe”，需要另行准备静态 Qt 并重新编译整个工程。
