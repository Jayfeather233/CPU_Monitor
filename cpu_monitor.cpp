#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <vector>
#include <iomanip>

// Function to parse /proc/stat and extract CPU times
std::vector<std::vector<unsigned long long>> parseProcStat()
{
    std::ifstream proc_stat("/proc/stat");
    std::string line;
    std::vector<std::vector<unsigned long long>> cpus_times;

    if (proc_stat.is_open())
    {
        while (std::getline(proc_stat, line))
        {
            std::istringstream iss(line);
            std::string cpu_label;
            iss >> cpu_label; // Read the "cpu" label
            if (cpu_label.find("cpu") != 0)
                break;

            unsigned long long time;
            std::vector<unsigned long long> cpu_times;
            while (iss >> time)
            {
                cpu_times.push_back(time);
            }
            cpus_times.push_back(cpu_times);
        }
    }

    return cpus_times;
}

// Function to calculate CPU usage
std::vector<double> calculateCpuUsage(const std::vector<std::vector<unsigned long long>> &prev_times, const std::vector<std::vector<unsigned long long>> &curr_times)
{
    std::vector<double> ret;
    for (size_t i = 0; i < prev_times.size(); ++i)
    {
        unsigned long long prev_idle = prev_times[i][3] + prev_times[i][4];
        unsigned long long curr_idle = curr_times[i][3] + curr_times[i][4];

        unsigned long long prev_total = 0;
        unsigned long long curr_total = 0;

        for (size_t j = 0; j < prev_times[i].size(); ++j)
        {
            prev_total += prev_times[i][j];
            curr_total += curr_times[i][j];
        }

        unsigned long long total_diff = curr_total - prev_total;
        unsigned long long idle_diff = curr_idle - prev_idle;
        ret.push_back(1.0 - static_cast<double>(idle_diff) / total_diff);
    }

    return ret;
}

void printInformation(const std::string &info)
{
    // Clear the previous output (works on most terminals)
    std::cout << "\033[2J\033[1;1H"; // Clear screen and move cursor to top-left

    // Print the information with newlines
    std::cout << info << std::endl;
}

void printProgressBar(std::ostream &ops, double percentage, int width = 50) {
    int numChars = static_cast<int>(percentage * width);
    ops << "[";
    for (int i = 0; i < width; ++i) {
        if (i < numChars)
            ops << "=";
        else
            ops << " ";
    }
    ops << "] " << std::setw(6) << std::setprecision(2) << std::fixed << (percentage * 100.0) << "%";
}

int main(int argc, char *argv[])
{
    size_t intev = 3; // 3 secs
    // Loop through arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        // Check for -n option
        if (arg == "-n")
        {
            // Check if there is a number provided after -n
            if (i + 1 < argc)
            {
                intev = atoi(argv[i + 1]);
                i++; // Move to the next argument
            }
            else
            {
                std::cout << "Error: -n option requires a number." << std::endl;
                return 1;
            }
        }

        // Check for --help option
        else if (arg == "--help")
        {
            std::cout << "Usage: " << argv[0] << " [-n number] [--help]" << std::endl
                      << "\t number is the interval." << std::endl;
            return 0;
        }

        // Invalid option
        else
        {
            std::cout << "Invalid option: " << arg << std::endl;
            return 1;
        }
    }

    std::vector<std::vector<unsigned long long>> prev_cpus_times;
    std::vector<std::vector<unsigned long long>> curr_cpus_times;
    std::cout << std::fixed << std::setprecision(2);
    while (true)
    {
        prev_cpus_times = parseProcStat();
        std::this_thread::sleep_for(std::chrono::seconds(intev));
        curr_cpus_times = parseProcStat();

        std::vector<double> cpu_usage = calculateCpuUsage(prev_cpus_times, curr_cpus_times);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "\rCPU Usage: " << cpu_usage[0]*100 << "%" << std::endl;
        for (int i = 1; i < cpu_usage.size(); ++i)
        {
            oss << "\rCPU" << i - 1 << " ";
            printProgressBar(oss, cpu_usage[i]);
            oss << std::endl;
        }
        printInformation(oss.str());

        // std::cout << prev_cpus_times.size() << ' ' << prev_cpus_times[0].size() << std::endl;
    }

    return 0;
}
