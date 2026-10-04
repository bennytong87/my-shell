# mysh - A Simple UNIX Shell in C

用 C 語言從零實作的 UNIX Shell，支援執行外部命令、內建命令、I/O 重定向、管道、背景執行，並用 SIGCHLD handler 自動回收殭屍程序。

## 功能

- 執行外部命令（ls、cat、echo 等）
- 內建命令：`cd`、`exit`
- I/O 重定向：`<`、`>`
- 單一管道：`|`
- 背景執行：`&`
- SIGCHLD handler 自動回收殭屍程序

## 編譯

```bash
make
```

或直接使用 gcc：

```bash
gcc -Wall -Wextra -g -o mysh mysh.c
```

## 執行

```bash
./mysh
```

## 使用範例

```bash
mysh> ls -l
mysh> echo hello world
mysh> echo hello > output.txt
mysh> cat < output.txt
mysh> ls | wc -l
mysh> cat file.txt | grep hello
mysh> sleep 30 &
mysh> cd /tmp
mysh> pwd
mysh> exit
```

## 架構

```
┌─────────────────────────────────────────────────────┐
│                     main loop                        │
│  1. 顯示提示符號 mysh>                               │
│  2. 讀取輸入 (fgets)                                 │
│  3. 移除換行 (strcspn)                               │
│  4. 切割 token (strtok)                              │
│  5. 檢查內建命令 (cd, exit)                          │
│  6. 檢查背景執行 (&)                                 │
│  7. 檢查管道 (|)                                     │
│  8. 分派：                                           │
│     - 有管道 → execute_pipe()                        │
│     - 無管道 → parse_redirect() + execute_command()  │
│  9. 回到步驟 1                                       │
└─────────────────────────────────────────────────────┘
```

## 系統呼叫

| 系統呼叫 | 用途 |
|----------|------|
| `fork()` | 複製 process |
| `execvp()` | 執行外部程式 |
| `waitpid()` | 等待 child 結束 |
| `pipe()` | 建立管道 |
| `dup2()` | 重定向檔案描述子 |
| `open()` | 開啟檔案 |
| `close()` | 關閉檔案描述子 |
| `chdir()` | 改變工作目錄 |
| `sigaction()` | 註冊 SIGCHLD handler |

## 技術

C 語言、Linux 系統程式設計、fork、exec、wait、pipe、dup2、open、close、signal、sigaction、SIGCHLD、檔案描述子

## 除錯工具

- `gdb`：追蹤 fork、exec、pipe
- `strace`：觀察系統呼叫
- `valgrind`：檢查記憶體泄漏

## 作者

Benny Tong
