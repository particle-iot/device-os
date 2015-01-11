Wip:
This contains the reflection test runner - it provides two executables - one the test driver, and the other a subject under test.



Based on the arduino unit test harness.
Common source code used to build both test driver and test subject. This ensures tests are executed in the same order.

Iterates through tests
Test driver may be older than the test subject. (So experimental features can be tested in the test subject.)
This means the test subject may have tests not in the test driver.
The default action of the driver is to wait for the test to be complete (timeout of 5 minutes)

Wont use the default runner in arduino unit, want to

1: in the driver, communicate with the subject to verify the next test
2. when the same, run the driver test code and the subject test
3. when different, run the default driver code, which is to wait for test ended from the test setup







