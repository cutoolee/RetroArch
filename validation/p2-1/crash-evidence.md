# P2.1 crash and ANR evidence

Status: `NOT_RUN`

Record the APK identity, device identity, test duration, and the exact logcat
command/filter used. Save the raw output outside this template and link it in
the table.

| ID | Scenario | Crash/ANR check | Result | Evidence |
|---|---|---|---|---|
| CRASH-01 | Launch → MENU → Back | fatal exception, force-close, ANR | NOT_RUN | |
| CRASH-02 | Save pending → repeated input → close | fatal exception, ANR, stuck UI | NOT_RUN | |
| CRASH-03 | Load pending → close/deinit | fatal exception, ANR, late callback fault | NOT_RUN | |
| CRASH-04 | Reset → Advanced → Exit | fatal exception, ANR, double-close | NOT_RUN | |
| CRASH-05 | P1 launch/core/content regression | fatal exception, ANR | NOT_RUN | |

Suggested capture command (adapt package and device as needed):

```sh
adb logcat -c
adb logcat -v threadtime > p2-1-logcat.txt
```

Stop capture after the scenario, then search for `FATAL EXCEPTION`, `ANR`,
`SIGSEGV`, `Abort message`, and `Force finishing`.
