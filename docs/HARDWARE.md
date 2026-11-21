# Display wiring and routing

The clock has three VK1640 drivers, each with a separate clock and data line.
The ESP32-C3 SuperMini uses GPIO 5/6 for U1, 7/10 for U2, and 20/21 for U4
(clock/data). The two button voltage ladders use GPIO 0 and GPIO 1.

The 42 × 9 matrix has three groups of 14 columns. In the unrotated view,
the first group is U4, the middle is U2, and the last is U1. Within a group,
columns 0–5 start at RAM bit 125 and step backward by one; each lower row
steps back six bits. Columns 6–13 start at bit 71 and step backward by one;
each lower row steps back eight bits. The five indicator bits are listed in
`LedMatrix.h`. The normal mounting orientation uses a 180° transform.

These physical routing facts were checked against
[kobebeefg/ikea-clock-lab](https://github.com/kobebeefg/ikea-clock-lab),
including its [KORTLINJE modification guide](https://github.com/kobebeefg/ikea-clock-lab/blob/main/tutorial/README.md).
That repository has no overall open source license. No source files from it
are included here.
