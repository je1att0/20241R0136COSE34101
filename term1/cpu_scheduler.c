#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define MAX_PROCESSES 100
#define MAX_IO_EVENTS 2

typedef enum
{
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} State;

typedef struct
{
    int pid;
    int arrival_time;
    int burst_time;
    int io_burst_time;
    int priority;
    int remaining_time;
    int start_time;
    int finish_time;
    int waiting_time;
    int turnaround_time;
    State state;
} Process;

typedef struct
{
    Process processes[MAX_PROCESSES];
    int count;
} ProcessQueue;

ProcessQueue readyQueue;
ProcessQueue waitingQueue;
Process originalProcesses[MAX_PROCESSES];

int io_times[MAX_IO_EVENTS];
int io_count = 0;

void initialize_queue(ProcessQueue *queue)
{
    queue->count = 0;
}

void enqueue(ProcessQueue *queue, Process p)
{
    queue->processes[queue->count++] = p;
}

Process dequeue(ProcessQueue *queue)
{
    Process p = queue->processes[0];
    for (int i = 0; i < queue->count - 1; i++)
    {
        queue->processes[i] = queue->processes[i + 1];
    }
    queue->count--;
    return p;
}

int is_empty(ProcessQueue *queue)
{
    return queue->count == 0;
}

Process create_process(int pid, int arrival_time, int burst_time, int io_burst_time, int priority)
{
    Process p;
    p.pid = pid;
    p.arrival_time = arrival_time;
    p.burst_time = burst_time;
    p.io_burst_time = io_burst_time;
    p.priority = priority;
    p.remaining_time = burst_time;
    p.start_time = -1;
    p.finish_time = -1;
    p.waiting_time = 0;
    p.turnaround_time = 0;
    p.state = NEW;
    return p;
}

int create_processes(ProcessQueue *queue, int num_processes)
{
    printf("\n=============== Created Processes ===============\n");
    printf("%-10s%-10s%-10s%-10s%-10s\n", "PID", "Arrival", "CPU", "I/O", "Priority");
    int total_time = 0;
    for (int i = 0; i < num_processes; i++)
    {
        int pid = i + 1;
        int arrival_time = rand() % 10;     // 0-9
        int burst_time = rand() % 10 + 1;   // 1-10
        int io_burst_time = rand() % 5 + 1; // 1-5
        int priority = rand() % 10;         // 0-9
        Process p = create_process(pid, arrival_time, burst_time, io_burst_time, priority);
        enqueue(queue, p);
        originalProcesses[i] = p;
        printf("%-10d%-10d%-10d%-10d%-10d\n", pid, arrival_time, burst_time, io_burst_time, priority);
        total_time += burst_time;
    }
    printf("==================================================\n\n");
    return total_time;
}

void generate_io_events(int total_time, int *io_times, int *io_count)
{
    printf("============== I/O Events Generated ==============\n");
    *io_count = rand() % 2 + 1; // 1, 또는 2회 I/O 발생
    int i = 0;

    while (i < *io_count)
    {
        int new_io_time = rand() % total_time;
        int is_unique = 1;
        for (int j = 0; j < i; j++)
        {
            if (io_times[j] == new_io_time)
            {
                is_unique = 0;
                break;
            }
        }
        if (is_unique)
        {
            io_times[i] = new_io_time;
            printf("I/O Event %d: %d\n", i + 1, io_times[i]);
            i++;
        }
    }
    printf("==================================================\n\n");
}

void reset_ready_queue()
{
    initialize_queue(&readyQueue);
    for (int i = 0; i < MAX_PROCESSES && originalProcesses[i].pid != 0; i++)
    {
        Process p = originalProcesses[i];
        p.state = NEW;
        p.remaining_time = p.burst_time;
        p.start_time = -1;
        p.finish_time = -1;
        p.waiting_time = 0;
        p.turnaround_time = 0;
        enqueue(&readyQueue, p);
    }
}

int handle_io_operations(ProcessQueue *readyQueue, ProcessQueue *waitingQueue, int current_time)
{
    int io_event_occurred = 0;
    for (int i = 0; i < waitingQueue->count; i++)
    {
        Process *p = &waitingQueue->processes[i];
        if (p->state == WAITING && current_time - p->finish_time >= p->io_burst_time)
        {
            p->state = READY;
            enqueue(readyQueue, *p);
            for (int j = i; j < waitingQueue->count - 1; j++)
            {
                waitingQueue->processes[j] = waitingQueue->processes[j + 1];
            }
            waitingQueue->count--;
            io_event_occurred = 1;
            i--;
        }
    }
    return io_event_occurred;
}

void print_gantt_chart(int timeline[], int n)
{
    printf("Gantt Chart:\n");
    for (int i = 0; i < n; i++)
    {
        if (timeline[i] == -1)
            printf("| -- ");
        else
            printf("| P%d ", timeline[i]);
    }
    printf("|\n");
    for (int i = 0; i <= n; i++)
    {
        printf("%-5d", i);
    }
    printf("\n");
    for (int i = 0; i < n; i++)
    {
        int io_event_found = 0;
        for (int k = 0; k < io_count; k++)
        {
            if (i == io_times[k])
            {
                printf("%-5s", "I/O");
                io_event_found = 1;
                break;
            }
        }
        if (!io_event_found)
        {
            printf("     ");
        }
    }
    printf("\n\n");
}

void evaluate(Process processes[], int num_processes)
{
    double total_waiting_time = 0;
    double total_turnaround_time = 0;
    for (int i = 0; i < num_processes; i++)
    {
        total_waiting_time += processes[i].waiting_time;
        total_turnaround_time += processes[i].turnaround_time;
        printf("PID %d - Waiting Time: %d, Turnaround Time: %d\n", processes[i].pid, processes[i].waiting_time, processes[i].turnaround_time);
    }
    printf("\nAverage Waiting Time: %2f\n", total_waiting_time / num_processes);
    printf("Average Turnaround Time: %2f\n", total_turnaround_time / num_processes);
    printf("==================================================\n\n");
}

void fcfs(ProcessQueue *queue)
{
    int current_time = 0;
    int completed_processes = 0;
    Process processes[MAX_PROCESSES];
    int timeline[1000]; 
    int timeline_index = 0;

    while (completed_processes < queue->count)
    {
        if (handle_io_operations(queue, &waitingQueue, current_time))
        {
            continue; // Check for I/O operations
        }

        int found_process = 0;
        for (int i = 0; i < queue->count; i++)
        {
            Process *p = &queue->processes[i];
            if (p->arrival_time <= current_time && p->remaining_time > 0)
            {
                found_process = 1;
                if (p->start_time == -1)
                {
                    p->start_time = current_time;
                }
                for (int j = 0; j < p->remaining_time; j++)
                {
                    timeline[timeline_index++] = p->pid;
                    current_time++;
                    if (handle_io_operations(queue, &waitingQueue, current_time))
                    {
                        break; // Check for I/O operations
                    }
                }
                p->remaining_time = 0;
                p->finish_time = current_time;
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                p->state = TERMINATED;
                processes[completed_processes++] = *p;
                break;
            }
        }
        if (!found_process)
        {
            timeline[timeline_index++] = -1; // IDLE
            current_time++;
        }
    }
    print_gantt_chart(timeline, timeline_index);
    evaluate(processes, queue->count);
}

void sjf(ProcessQueue *queue, int preemptive)
{
    int current_time = 0;
    int completed_process = 0;
    Process processes[MAX_PROCESSES];
    int timeline[1000]; 
    int timeline_index = 0;

    while (completed_process < queue->count)
    {
        if (handle_io_operations(queue, &waitingQueue, current_time))
        {
            continue; // Check for I/O operations
        }

        int shortest_time = INT_MAX;
        int shortest_index = -1;

        for (int i = 0; i < queue->count; i++)
        {
            Process *p = &queue->processes[i];
            if (p->arrival_time <= current_time && p->remaining_time > 0 && p->remaining_time < shortest_time)
            {
                shortest_time = p->remaining_time;
                shortest_index = i;
            }
        }

        if (shortest_index == -1)
        {
            timeline[timeline_index++] = -1; // IDLE
            current_time++;
            continue;
        }

        Process *p = &queue->processes[shortest_index];

        if (p->start_time == -1)
        {
            p->start_time = current_time;
        }

        if (preemptive)
        {
            timeline[timeline_index++] = p->pid;
            current_time++;
            p->remaining_time--;
            if (p->remaining_time == 0)
            {
                p->finish_time = current_time;
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                p->state = TERMINATED;
                processes[completed_process++] = *p;
            }
        }
        else
        {
            for (int j = 0; j < p->remaining_time; j++)
            {
                timeline[timeline_index++] = p->pid;
                current_time++;
                if (handle_io_operations(queue, &waitingQueue, current_time))
                {
                    break; // Check for I/O operations
                }
            }
            p->remaining_time = 0;
            p->finish_time = current_time;
            p->turnaround_time = p->finish_time - p->arrival_time;
            p->waiting_time = p->turnaround_time - p->burst_time;
            p->state = TERMINATED;
            processes[completed_process++] = *p;
        }
    }
    print_gantt_chart(timeline, timeline_index);
    evaluate(processes, queue->count);
}

void priority_scheduling(ProcessQueue *queue, int preemptive)
{
    int current_time = 0;
    int completed_processes = 0;
    Process processes[MAX_PROCESSES];
    int timeline[1000]; 
    int timeline_index = 0;

    while (completed_processes < queue->count)
    {
        if (handle_io_operations(queue, &waitingQueue, current_time))
        {
            continue; // Check for I/O operations
        }

        int highest_priority = INT_MAX;
        int highest_index = -1;

        for (int i = 0; i < queue->count; i++)
        {
            Process *p = &queue->processes[i];
            if (p->arrival_time <= current_time && p->remaining_time > 0 && p->priority < highest_priority)
            {
                highest_priority = p->priority;
                highest_index = i;
            }
        }

        if (highest_index == -1)
        {
            timeline[timeline_index++] = -1; // IDLE
            current_time++;
            continue;
        }

        Process *p = &queue->processes[highest_index];

        if (p->start_time == -1)
        {
            p->start_time = current_time;
        }

        if (preemptive)
        {
            timeline[timeline_index++] = p->pid;
            current_time++;
            p->remaining_time--;
            if (p->remaining_time == 0)
            {
                p->finish_time = current_time;
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                p->state = TERMINATED;
                processes[completed_processes++] = *p;
            }
        }
        else
        {
            for (int j = 0; j < p->remaining_time; j++)
            {
                timeline[timeline_index++] = p->pid;
                current_time++;
                if (handle_io_operations(queue, &waitingQueue, current_time))
                {
                    break; // Check for I/O operations
                }
            }
            p->remaining_time = 0;
            p->finish_time = current_time;
            p->turnaround_time = p->finish_time - p->arrival_time;
            p->waiting_time = p->turnaround_time - p->burst_time;
            p->state = TERMINATED;
            processes[completed_processes++] = *p;
        }
    }

    print_gantt_chart(timeline, timeline_index);
    evaluate(processes, queue->count);
}

void round_robin(ProcessQueue *queue, int time_quantum)
{
    int current_time = 0;
    int completed_processes = 0;
    Process processes[MAX_PROCESSES];
    int timeline[1000]; 
    int timeline_index = 0;
    int total_processes = queue->count;

    while (completed_processes < total_processes)
    {
        if (handle_io_operations(queue, &waitingQueue, current_time))
        {
            continue; // Check for I/O operations
        }

        int queue_count = queue->count;
        int idle = 1; // idle 상태를 나타내는 변수

        for (int i = 0; i < queue_count; i++)
        {
            Process p = dequeue(queue);
            if (p.arrival_time <= current_time && p.remaining_time > 0)
            {
                idle = 0; // 실행 가능한 프로세스가 있음
                if (p.start_time == -1)
                {
                    p.start_time = current_time;
                }
                int time_slice = (p.remaining_time > time_quantum) ? time_quantum : p.remaining_time;
                for (int j = 0; j < time_slice; j++)
                {
                    timeline[timeline_index++] = p.pid;
                    current_time++;
                    if (handle_io_operations(queue, &waitingQueue, current_time))
                    {
                        break; // Check for I/O operations
                    }
                }
                p.remaining_time -= time_slice;

                if (p.remaining_time == 0)
                {
                    p.finish_time = current_time;
                    p.turnaround_time = p.finish_time - p.arrival_time;
                    p.waiting_time = p.turnaround_time - p.burst_time;
                    p.state = TERMINATED;
                    processes[completed_processes++] = p;
                }
                else
                {
                    enqueue(queue, p);
                }
            }
            else
            {
                enqueue(queue, p);
            }
        }

        if (idle)
        {
            timeline[timeline_index++] = -1; // IDLE
            current_time++; // idle 상태일 때만 current_time 증가
        }
    }
    print_gantt_chart(timeline, timeline_index);
    evaluate(processes, completed_processes);
}

/*
void compare() {

}
*/

int main()
{
    srand((unsigned)time(NULL));
    int num_processes = 0;
    printf("Number of processes: ");
    scanf("%d", &num_processes);
    int time_quantum = 0;
    printf("Time Quantum: ");
    scanf("%d", &time_quantum);

    int total_time = 0;
    total_time = create_processes(&readyQueue, num_processes);

    generate_io_events(total_time, io_times, &io_count);

    while (1)
    {
        printf("============== Scheduling Algorithm ==============\n");
        printf("1. FCFS\n");
        printf("2. SJF (Non-preemptive)\n");
        printf("3. SJF (Preemptive)\n");
        printf("4. Priority (Non-preemptive)\n");
        printf("5. Priority (Preemptive)\n");
        printf("6. Round Robin\n");
        printf("7. Finished\n");
        printf("==================================================\n\n");
        printf("Select scheduling algorithm: ");
        int choice = 0;
        scanf("%d", &choice);
        printf("\n");

        if (choice == 7)
        {
            // compare();
            break;
        }

        reset_ready_queue();

        switch (choice)
        {
        case 1:
            printf("================= FCFS Algorithm =================\n");
            fcfs(&readyQueue);
            break;
        case 2:
            printf("========== Non-preemptive SFJ Algorithm ==========\n");
            sjf(&readyQueue, 0);
            break;
        case 3:
            printf("============ Preemptive SFJ Algorithm =============\n");
            sjf(&readyQueue, 1);
            break;
        case 4:
            printf("======== Non-preemptive Priority Algorithm ========\n");
            priority_scheduling(&readyQueue, 0);
            break;
        case 5:
            printf("========== Preemptive Priority Algorithm ==========\n");
            priority_scheduling(&readyQueue, 1);
            break;
        case 6:
            printf("============== Round Robin Algorithm ==============\n");
            round_robin(&readyQueue, time_quantum);
            break;
        default:
            printf("Invalid choice\n");
            break;
        }
    }

    return 0;
}