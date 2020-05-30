#include <iostream>
#include <mpi.h>
#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <vector>
#include <fstream>
#include <cstdlib>

using matrix_t = std::vector<std::vector<double>>;

matrix_t load_csv(const std::string input_csv_file)
{
    PLOG_INFO << "Loading matrix";
    std::ifstream data(input_csv_file);
    std::string line;
    // Skipping required dimension lines
    std::getline(data, line);
    matrix_t parsed_csv;
    while (std::getline(data, line))
    {
        std::vector<double> parsedRow;
        std::stringstream s(line);
        std::string cell;
        while (std::getline(s, cell, ';'))
        {
            double c = atof(cell.c_str());
            parsedRow.push_back(c);
        }

        parsed_csv.push_back(parsedRow);
    }
    return parsed_csv;
}

void print_matrix(matrix_t matrix, int node, std::string matrix_name)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    for (std::vector<double> row : matrix)
    {
        for (double cell : row)
        {
            oss << cell << ";";
        }
        oss << "\n";
    }
    oss << "Node: " << node << " Matrix: " << matrix_name;
    PLOG_INFO << oss.str();
}

matrix_t multiply_matrixes(matrix_t matrix_a, matrix_t matrix_b)
{

    matrix_t output_matrix(matrix_a.size(), std::vector<double>(matrix_b.at(0).size()));
    int output_rows = output_matrix.size();
    int output_columns = output_matrix.at(0).size();
    int inner_size = matrix_b.size();
    printf("Multiplying matrixes using sequential method\n");
    for (int row = 0; row < output_rows; row++)
    {
        for (int col = 0; col < output_columns; col++)
        {
            for (int inner = 0; inner < inner_size; inner++)
            {
                output_matrix[row][col] += matrix_a[row][inner] * matrix_b[inner][col];
            }
        }
    }
    //print_matrix(output_matrix, 1, "output matrix");
    return output_matrix;
}

void send_matrix(const matrix_t &matrix_to_send, int target_node, int start_row, int end_row, int column_count)
{
    MPI_Send(&start_row, 1, MPI_INT, target_node, 0, MPI_COMM_WORLD);
    MPI_Send(&end_row, 1, MPI_INT, target_node, 0, MPI_COMM_WORLD);
    MPI_Send(&column_count, 1, MPI_INT, target_node, 0, MPI_COMM_WORLD);
    for (int current_row = start_row; current_row <= end_row; current_row++)
    {
        MPI_Send(matrix_to_send.at(current_row).data(), column_count, MPI_DOUBLE, target_node, 0, MPI_COMM_WORLD);
    }
}

matrix_t receive_matrix(int source_node)
{
    matrix_t output_matrix;
    int local_start = 0;
    int local_end = 0;
    int local_col_count = 0;
    MPI_Recv(&local_start, 1, MPI_INT, source_node, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&local_end, 1, MPI_INT, source_node, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&local_col_count, 1, MPI_INT, source_node, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    for (int current_row = local_start; current_row <= local_end; current_row++)
    {
        std::vector<double> local_entry;
        local_entry.resize(local_col_count);
        MPI_Recv(local_entry.data(), local_col_count, MPI_DOUBLE, source_node, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        output_matrix.push_back(local_entry);
    }
    return output_matrix;
}

void send_broadcast(matrix_t matrix_to_send)
{
    int column_count = matrix_to_send.at(0).size();
    int row_count = matrix_to_send.size();
    MPI_Bcast(&row_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&column_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    for (int current_row = 0; current_row < row_count; current_row++)
    {
        MPI_Bcast(matrix_to_send.at(current_row).data(), column_count, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }
}

matrix_t receive_broadcast()
{
    matrix_t received_matrix;
    int local_b_col_count = 0;
    int local_b_row_count = 0;
    MPI_Bcast(&local_b_row_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&local_b_col_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    for (int current_row = 0; current_row < local_b_row_count; current_row++)
    {
        std::vector<double> local_entry_b;
        local_entry_b.resize(local_b_col_count);
        MPI_Bcast(local_entry_b.data(), local_b_col_count, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        received_matrix.push_back(local_entry_b);
    }
    return received_matrix;
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
        matrix_t matrix_a = load_csv("a.csv");
        matrix_t matrix_b = load_csv("b.csv");
        int matrix_size = matrix_a.size();
        PLOG_INFO << "Getting input values";
        startTime = MPI_Wtime();
        // Start processing on node 0 sequentially

        endTime = MPI_Wtime();
        timeSingle = endTime - startTime;
        startTime = MPI_Wtime();
        PLOG_INFO << "Sending data to other nodes";
        int rest = matrix_size % numOfNodes;
        int chunk = matrix_size / numOfNodes;
        int row_count = matrix_a.size();
        int column_count = matrix_a.at(0).size();
        PLOG_INFO << "Rest from division: " << rest;
        for (int currNodeNum = 1; currNodeNum < numOfNodes; currNodeNum++)
        {
            int start = (currNodeNum - 1) * chunk;
            int end = currNodeNum * chunk - 1;
            send_matrix(matrix_a, currNodeNum, start, end, column_count);
        }
        send_broadcast(matrix_b);
        int local_start = (numOfNodes - 1) * chunk;

        int local_end = matrix_size;
        matrix_t local_matrix_a = matrix_t(matrix_a.begin() + local_start, matrix_a.end());
        matrix_t output_matrix = multiply_matrixes(local_matrix_a, matrix_b);
        for (int currNodeNum = 1; currNodeNum < numOfNodes; currNodeNum++)
        {
            matrix_t out_matrix = receive_matrix(currNodeNum);
            //print_matrix(out_matrix, 0, "Received output matrix");
        }
    }
    else
    {
        int local_start = 0;
        int local_end = 0;
        int local_col_count = 0;
        matrix_t local_matrix_a = receive_matrix(0);
        PLOG_INFO << "Loaded matrix a for node " << node;
        matrix_t local_matrix_b = receive_broadcast();
        PLOG_INFO << "Loaded matrix b for node " << node;
        matrix_t output_matrix = multiply_matrixes(local_matrix_a, local_matrix_b);
        send_matrix(output_matrix, 0, 0, output_matrix.size() - 1, output_matrix.at(0).size());
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