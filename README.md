# coc-bank-queue
Bank Queue Simulator
Overview
This project simulates an 8-hour day at a bank to analyze queue performance and customer waiting times.  
Customers arrive according to a **Poisson process**, are served by multiple tellers with random service durations, and their waiting times are recorded for statistical analysis.


Features
- **Poisson arrival model** for realistic random customer flow  
- **Multiple tellers** working in parallel  
- **Dynamic queue** implemented with linked lists  
- **Dynamic memory allocation** for scalable storage of wait times  
- Automatic computation of:
  - Mean (average)
  - Median
  - Mode
  - Standard deviation
  - Longest wait time


Simulation Details

| Parameter            | Description                          | Default               |
|--------------------- |------------------------------------- |---------------------- |
| `SIMULATION_MINUTES` | Total time simulated                 | 480 minutes (8 hours) |
| `MIN_SERVICE_TIME`   | Minimum service duration             | 2 minutes             |
| `MAX_SERVICE_TIME`   | Maximum service duration             | 3 minutes             |
| `λ (lambda)`         | Average customer arrivals per minute | User input            |
| `num_tellers`        | Number of tellers working            | User input            |


How It Works
1. At each simulated minute:
   - New customers arrive based on a **Poisson random generator**.
   - Free tellers serve waiting customers.
   - Each teller's service time counts down per minute.
2. When a teller finishes, they serve the next waiting customer.
3. All customer wait times are recorded dynamically.
4. After 480 minutes, statistical data is calculated and displayed.

Data Structures Used
- **Linked List** – manages the queue (FIFO order)
- **Dynamic Array (realloc)** – stores wait times
- **Structs**:
  - `Customer` – individual queue entry
  - `Queue` – queue manager
  - `Teller` – teller state
  - `WaitTimeStorage` – dynamic storage of wait times



gcc bank_queue_simulator.c -o bank_sim -lm
This C program simulates an 8-hour bank operation using a discrete-event queue model. Customers arrive following a Poisson distribution, and multiple tellers serve them with random service times between 2 and 3 minutes. The simulation uses linked lists to manage the queue and a dynamic array to store customer wait times.

It computes detailed post-simulation statistics including:

Mean, Median, Mode, Standard Deviation, and Maximum Wait Time.

The simulation helps analyze how varying the arrival rate (λ) and number of tellers affects customer waiting times and service efficiency.
