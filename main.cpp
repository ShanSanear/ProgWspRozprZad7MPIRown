#include <iostream>
#include <mpi.h>
#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <vector>
#include <fstream>
#include <cstdlib>

//using matrix = std::vector<std::vector<double>>;

std::vector<std::vector<double> > load_csv(const std::string input_csv_file) {
    PLOG_INFO << "Loading matrix";
    std::ifstream data(input_csv_file);
    std::string line;
    // Skipping required dimension lines
    std::getline(data, line);
    std::vector<std::vector<double> > parsed_csv;
    while (std::getline(data, line)) {
        std::vector<double> parsedRow;
        std::stringstream s(line);
        std::string cell;
        while (std::getline(s, cell, ';')) {
            double c = atof(cell.c_str());
            parsedRow.push_back(c);
        }

        parsed_csv.push_back(parsedRow);
    }
    return parsed_csv;
}

double get_double_from_stdin(const char *message)
{
    double out;
    std::cout << message << std::endl;
    std::cin >> out;
    return out;
}

int get_int_from_stdin(const char *message)
{
    int out;
    std::cout << message << std::endl;
    std::cin >> out;
    return out;
}

int main()
{
    int standardPrecision = 6;
    const int struct_size = 7;
    double startTime, endTime, parallelTimeTaken, timeSingle;
    plog::RollingFileAppender<plog::TxtFormatter> fileAppender("Datalogger.txt", 1048576, 5);
    plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::debug, &fileAppender).addAppender(&consoleAppender);
    int node, numOfNodes;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &numOfNodes);
    MPI_Comm_rank(MPI_COMM_WORLD, &node);
    if (node == 0)
    {
        std::vector<std::vector <double> > matrix = load_csv("a.csv");
        int matrix_size = matrix.size();
        PLOG_INFO << "Getting input values";
        startTime = MPI_Wtime();
        // Start processing on node 0 sequentially
        
        endTime = MPI_Wtime();
        timeSingle = endTime - startTime;
        startTime = MPI_Wtime();
        PLOG_INFO << "Sending data to other nodes";
        int rest = matrix_size % numOfNodes;
        int chunk = matrix_size / numOfNodes;
        int row_count = matrix.size();
        int column_count = matrix.at(0).size();
        PLOG_INFO << "Rest from division: " << rest;
        for (int currNodeNum = 1; currNodeNum < numOfNodes; currNodeNum++)
        {
            int start = (currNodeNum - 1) * chunk;
            int end = currNodeNum * chunk - 1;
            MPI_Send(&start, 1, MPI_INT, currNodeNum, 0, MPI_COMM_WORLD);
            MPI_Send(&end, 1, MPI_INT, currNodeNum, 0, MPI_COMM_WORLD);
            //MPI_Send(&row_count, 1, MPI_INT, currNodeNum, 0, MPI_COMM_WORLD);
            MPI_Send(&column_count, 1, MPI_INT, currNodeNum, 0, MPI_COMM_WORLD);
            for (int current_row = start; current_row <= end; current_row++) {
                MPI_Send(matrix.at(current_row).data(), column_count, MPI_DOUBLE, currNodeNum, 0, MPI_COMM_WORLD);
            }

        }
        int local_start = (numOfNodes-1)*chunk;
        
        int local_end = matrix_size;
        PLOG_DEBUG << "Start: " << local_start << " end: " << local_end << " node number: " << node;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        for (int current_row = local_start; current_row < local_end; current_row++) {
            for (int current_col = 0; current_col < column_count; current_col++) {
                oss << matrix[current_row][current_col] << ";";
            }
            oss << "\n";
        }
        
        PLOG_INFO << oss.str();
    }
    else
    {
        int local_start = 0;
        int local_end = 0;
        int local_col_count = 0;
        std::vector<std::vector <double> > local_matrix;
        std::vector <double> local_entry;
        MPI_Recv(&local_start, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&local_end, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&local_col_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        PLOG_DEBUG << "Start: " << local_start << " end: " << local_end << " node number: " << node;
        for (int current_row = local_start; current_row <= local_end; current_row++) {
            std::vector <double> local_entry;
            local_entry.resize(local_col_count);
            MPI_Recv(local_entry.data(), local_col_count, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            local_matrix.push_back(local_entry);
        }
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        for (std::vector<double> row : local_matrix) {
            for (double cell : row ) {
                oss << cell << ";";
            }
            oss << "\n";
        }
        PLOG_INFO << oss.str();
    }
    PLOG_INFO << "Finished processing, node: " << node;

  
    MPI_Barrier(MPI_COMM_WORLD);
    if (node == 0)
    {
        //Postprocessing
        endTime = MPI_Wtime();
        parallelTimeTaken = endTime - startTime;
        PLOG_INFO << "Parallized time: " << parallelTimeTaken << " second(s)";
    }
    MPI_Finalize();
    return 0;
}