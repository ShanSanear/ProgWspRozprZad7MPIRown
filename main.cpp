#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <iterator>

#include <mpi.h>

#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>

using matrix_t = std::vector<std::vector<double>>;

void print_matrix(const matrix_t& matrix, int node, const std::string& matrix_name)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    for (const std::vector<double>& row : matrix)
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

std::string get_string_from_cin(const std::string& prompt) {
    printf("%s\n", prompt.c_str());
    std::string value;
    std::cin >> value;
    return value;
}

matrix_t load_csv(const std::string& input_csv_file)
{
    PLOG_INFO << "Loading matrix from path: " << input_csv_file;
    std::ifstream data(input_csv_file);
    std::string line;
    // Skipping required dimension lines
    std::getline(data, line);
    // Checking if file contains second dimensional line - if it does not, go back
    std::streampos curr_position = data.tellg();
    std::getline(data, line);
    if (line.find(';') != std::string::npos) {
        data.seekg(curr_position, std::ios_base::beg);
    }
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

matrix_t multiply_matrixes(matrix_t matrix_a, matrix_t matrix_b)
{
    matrix_t output_matrix(matrix_a.size(), std::vector<double>(matrix_b.at(0).size()));
    int output_rows = output_matrix.size();
    int output_columns = output_matrix.at(0).size();
    int inner_size = matrix_b.size();
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
    return output_matrix;
}

void send_matrix(const matrix_t &matrix_to_send, int target_node, size_t start_row, size_t end_row, size_t column_count)
{
    MPI_Send(&start_row, 1, MPI_INT, target_node, 0, MPI_COMM_WORLD);
    MPI_Send(&end_row, 1, MPI_INT, target_node, 0, MPI_COMM_WORLD);
    MPI_Send(&column_count, 1, MPI_INT, target_node, 0, MPI_COMM_WORLD);
    for (int row_idx = start_row; row_idx <= end_row; row_idx++)
    {
        MPI_Send(matrix_to_send.at(row_idx).data(), column_count, MPI_DOUBLE, target_node, 0, MPI_COMM_WORLD);
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
        std::vector<double> matrix_row;
        matrix_row.resize(local_col_count);
        MPI_Recv(matrix_row.data(), local_col_count, MPI_DOUBLE, source_node, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        output_matrix.push_back(matrix_row);
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

void save_matrix(const matrix_t &matrix_to_save, const std::string &output_file_path) {
    std::ofstream output(output_file_path);
    PLOG_INFO << "Saving file under path: " << output_file_path;
    output << matrix_to_save.size() << std::endl;
    output << matrix_to_save.at(0).size() << std::endl;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);
    for (auto row : matrix_to_save) {
        oss.str(std::string());
        std::copy(row.begin(), row.end() - 1, std::ostream_iterator<double>(oss, ";"));
        std::copy(row.end() - 1, row.end(), std::ostream_iterator<double>(oss));
        output << oss.str() << std::endl;
    }
}

void run_sequentially(const matrix_t& matrix_a, const matrix_t& matrix_b) {
    matrix_t output_matrix(matrix_a.size(), std::vector<double>(matrix_b.at(0).size()));
    int output_rows = output_matrix.size();
    int output_columns = output_matrix.at(0).size();
    int inner_size = matrix_b.size();
    PLOG_INFO << "Multiplying matrixes using sequential method";
    for (int row = 0; row < output_rows; row++) {
        for (int col = 0; col < output_columns; col++) {
            for (int inner = 0; inner < inner_size; inner++) {
                output_matrix[row][col] += matrix_a[row][inner] * matrix_b[inner][col];
            }
        }
    }
}

void receive_output_from_nodes(int num_of_nodes, matrix_t &final_matrix) {
    for (int currNodeNum = 1; currNodeNum < num_of_nodes; currNodeNum++)
    {
        matrix_t out_matrix = receive_matrix(currNodeNum);
        final_matrix.insert(final_matrix.end(), out_matrix.begin(), out_matrix.end());
    }
}

matrix_t process_master_node_parallel(int num_of_nodes, matrix_t &matrix_a, const matrix_t &matrix_b,
                                      int matrix_size) {
    PLOG_INFO << "Sending data to other nodes";
    matrix_t final_matrix;
    int chunk = matrix_size / num_of_nodes;
    int row_count = matrix_a.size();
    int column_count = matrix_a.at(0).size();
    for (int currNodeNum = 1; currNodeNum < num_of_nodes; currNodeNum++)
    {
        int start = (currNodeNum - 1) * chunk;
        int end = currNodeNum * chunk - 1;
        send_matrix(matrix_a, currNodeNum, start, end, column_count);
    }
    send_broadcast(matrix_b);

    // This will also include the "leftovers" at the end of the matrix, that couldn't be evenly distributed
    int local_start = (num_of_nodes - 1) * chunk;
    int local_end = matrix_size;
    matrix_t local_matrix_a = matrix_t(matrix_a.begin() + local_start, matrix_a.end());
    matrix_t local_output_matrix = multiply_matrixes(local_matrix_a, matrix_b);

    receive_output_from_nodes(num_of_nodes, final_matrix);
    final_matrix.insert(final_matrix.end(), local_output_matrix.begin(), local_output_matrix.end());
    return final_matrix;
}

void process_matrix_other_nodes(int node) {
    matrix_t local_matrix_a = receive_matrix(0);
    PLOG_INFO << "Loaded matrix a for node " << node;
    matrix_t local_matrix_b = receive_broadcast();
    PLOG_INFO << "Loaded matrix b for node " << node;
    matrix_t output_matrix = multiply_matrixes(local_matrix_a, local_matrix_b);
    PLOG_INFO << "Processed output matrix for node " << node;
    send_matrix(output_matrix, 0, 0, output_matrix.size() - 1, output_matrix.at(0).size());
    PLOG_INFO << "Sent output matrix from node " << node;
}

int main()
{
    int node, num_of_nodes;
    double start_time, end_time, parallel_time, sequential_time;
    matrix_t final_matrix;

    plog::RollingFileAppender<plog::TxtFormatter> fileAppender("Datalogger.txt", 1048576, 5);
    plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::debug, &fileAppender).addAppender(&consoleAppender);

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_of_nodes);
    MPI_Comm_rank(MPI_COMM_WORLD, &node);
    if (node == 0)
    {
        std::string matrix_a_path = get_string_from_cin("Provide path for matrix A:");
        std::string matrix_b_path = get_string_from_cin("Provide path for Matrix B:");
        PLOG_INFO << "Path for matrix a: " << matrix_a_path;
        PLOG_INFO << "Path for matrix b: " << matrix_b_path;
        matrix_t matrix_a = load_csv(matrix_a_path);
        matrix_t matrix_b = load_csv(matrix_b_path);
        int matrix_size = matrix_a.size();
        start_time = MPI_Wtime();
        // Start processing on node 0 sequentially
        run_sequentially(matrix_a, matrix_b);
        end_time = MPI_Wtime();
        sequential_time = end_time - start_time;
        start_time = MPI_Wtime();
        final_matrix = process_master_node_parallel(num_of_nodes, matrix_a, matrix_b, matrix_size);
    }
    else
    {
        process_matrix_other_nodes(node);
    }
    PLOG_INFO << "Finished processing, node: " << node;

    MPI_Barrier(MPI_COMM_WORLD);
    if (node == 0)
    {
        //Postprocessing
        end_time = MPI_Wtime();
        parallel_time = end_time - start_time;
        PLOG_INFO << "Parallized time: " << parallel_time << " second(s)";
        PLOG_INFO << "Serialized time taken: " << sequential_time << " second(s)";
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4);
        oss << "C_" << sequential_time << "_" << parallel_time << ".csv";
        save_matrix(final_matrix, oss.str());
    }
    MPI_Finalize();
    return 0;
}

/*
Implementacja została wykonana dla dowolnej liczby węzłów oraz dla macierzy o dowolnych rozmiarach
Działa dla obu formatów danych wejściowych, gdyż dane wczytywane są dynamicznie
*/