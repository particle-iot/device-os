Cloud API Test Plan
===================

`Particle.publishVitals`
------------------------

**Given:**

- The test application `<particle-iot/device-os>/user/tests/app/particle_publish_vitals/particle_publish_vitals.cpp`
- The Particle CLI installed on local machine.

**When:**

- The application is compiled, installed and launched
- You have subscribed to your messages via `particle subscribe mine`

**Then:**

- `PLATFORM_THREADING == 0`

  Confirm four describe messages are printed to the console.

  1. `Particle.publishVitals(n)` | _n_ > 0
  1. `Particle.publishVitals(n)` | _n_ == 0
  1. `Particle.publishVitals(n)` | _n_ == `particle::NOW`
  1. `Particle.publishVitals()`

- `PLATFORM_THREADING == 1`

  Confirm seven describe messages are printed to the console.

  1. `Particle.publishVitals(n)` | _n_ > 0
  1. -- periodic publish 1 (@ _n_ secs)
  1. -- periodic publish 2 (@ 2_n_ secs)
  1. -- periodic publish 3 (@ 3_n_ secs)
  1. `Particle.publishVitals(n)` | _n_ == 0
  1. `Particle.publishVitals(n)` | _n_ == `particle::NOW`
  1. `Particle.publishVitals()`
