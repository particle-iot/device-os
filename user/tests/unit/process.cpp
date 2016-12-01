#include "catch.hpp"
#include "spark_wiring_process.h"

SCENARIO("Start a subprocess", "[process]")
{
    Process proc = Process::run("/bin/false");
    proc.wait();
    REQUIRE(proc.exited() == true);
    REQUIRE(proc.exitCode() == 1);
}

SCENARIO("Waits for process to finish", "[process]")
{
    Process proc = Process::run("bc");
    REQUIRE(proc.exited() == false);
    proc.in().close();
    proc.wait();
    REQUIRE(proc.exited() == true);
}

SCENARIO("Captures standard output", "[process]")
{
    Process proc = Process::run("echo 42 followed by something long");
    proc.wait();
    int result = proc.out().parseInt();
    REQUIRE(result == 42);
}

SCENARIO("Accepts standard input", "[process]")
{
    Process proc = Process::run("bc");
    proc.in().print("6 * 7\n");
    proc.in().close();
    proc.wait();
    int result = proc.out().parseInt();
    REQUIRE(result == 42);
}

SCENARIO("Captures standard error", "[process]")
{
    Process proc = Process::run("./script_out_err.sh");
    proc.wait();
    proc.out().setTimeout(1);
    proc.err().setTimeout(1);
    String out = proc.out().readString();
    String err = proc.err().readString();
    REQUIRE(out.equals("This is ok\n") == true);
    REQUIRE(err.equals("This is an error\n") == true);
    REQUIRE(proc.exitCode() == 3);
}

SCENARIO("Returns an error when a command is not found", "[process]")
{
    Process proc = Process::run("iawghoigwaehogwhaiooihgwahiogwe");
    proc.wait();
    REQUIRE(proc.exitCode() == Process::COMMAND_NOT_FOUND);
}
