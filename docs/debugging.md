
## Debug Build

The firmware includes a debugging aid that enables debug output, from the system and from your own application code.

To create a debug build, add `DEBUG_BUILD=y` to the `make` command line. If the previous build was not a debug build then
you should add `clean` to perform a clean build.

On the photon, the system modules must also be rebuilt also with `DEBUG_BUILD` set.


### Logging Messages

Logging messages enabled in the debug build of firmware.

A debug output handler determines where the log messages are printed to.
The system provides some built-in output handlers:

- `SerialDebugOutput` writes output to USB Serial
- `Serial1DebugOutput` writes output to Hardware Serial (Serial1)

To add an output handler to your application, declare it at top of your application code (before setup):

```
SerialDebugOutput debugOutput;
```

This will print all log messages to USB Serial.

The debug output variable can take two optional parameters

- the baud rate,
- the logging level - filters out messages below a given level

```
SerialDebugOutput debugOutput;          // default is 9600 and log everything
SerialDebugOutput debugOutput(57600);   // use a faster baudrate and log everything
SerialDebugOutput debugOutput(57600, WARN_LEVEL); // use a faster baudrate and log only warnings or more severe

```

### Log Levels

These log levels are available:

```
    ALL_LEVEL           log everything
    TRACE_LEVEL
    DEBUG_LEVEL
    WARN_LEVEL
    ERROR_LEVEL
    PANIC_LEVEL
    NO_LOG_LEVEL        log nothing
```

When a log level is set on the debug output, only log messages that are at the same level or
further down in the list are printed.

For example, if the log level is set to `WARN_LEVEL` then warnings, errors and
panic events are logged, but not debug messages or


### Adding your own log messages

Logging messages are added to your application code by using a logging macro.

```
void loop()
{
    static unsigned count = 0;
    DEBUG("loop count %d", count);
    count++;
}
```
If the debug level is enabled, this would print out "loop count 0" on the first
iteration, and higher numbers with each iteration.

(Note that we increment the variable outside of the logging macro. This is because
logging macros shouldn't have side affects that the program depends on, since
these side affect will not be present in the non-debug build of the code.)

The logging message includes the


The name of the macro sets the logging level.

```
    DEBUG
    INFO
    WARN
    ERROR
```

The first parameter is a format string. This follows the `printf` format.
The remaining parameters are substituted into the placeholders in the format string.



