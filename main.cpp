#include <iostream>
#include <mpi.h>
#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <vector>
#include <fstream>
#include <cstdlib>

//using matrix = std::vector<std::vector<double>>;

std::vector<std::vector<double> > load_csv(const std::string input_csv_file) {
    std::ifstream data(input_csv_file.c_str());
    std::string line;
    // Skipping required dimension lines
    std::getline(data, line);
    std::vector<std::vector<double> > parsed_csv;
    while (std::getline(data, line)) {
        std::vector<double> parsedRow;
        std::stringstream s(line);
        std::string cell;
        while (std::getline(s, cell, ';')) {
            parsedRow.push_back(atof(cell.c_str()));
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
    // Change datatype
    
    // End of creating datatype
    double C[3][3] = { 0 };
    
    std::vector<std::vector <double> > matrix;
    for (int i = 0; i < 10; i++) {
        std::vector<double> entry;
        for (int j = 0; j < 10; j++) {
            entry.push_back((double) j);
        }
        matrix.push_back(entry);
    }
    std::vector<double> simple_entry;
    for (int j = 0; j < 10; j++) {
        simple_entry.push_back((double) j);
    }

    int matrix_size = matrix.size();
    //std::stringstream resultStream;
    //resultStream << std::fixed << std::setprecision(standardPrecision);
    if (node == 0)
    {
        PLOG_INFO << "Getting input values";
        //get input parameters
        // double A[3][3] = { {1,2,3}, {4,5,6}, {7,8,9} };
        // double B[3][3] = { {1,1,1}, {2,2,2}, {3,3,3} };

        

        startTime = MPI_Wtime();
        // Start processing on node 0 sequentially
        
        endTime = MPI_Wtime();
        timeSingle = endTime - startTime;
        // TODO rename this string
        //PLOG_INFO << resultStream.str();
        //PLOG_INFO << "Single node time: " << timeSingle << " second(s)";
        //resultStream << std::setprecision(standardPrecision);
        // prepare structure
        startTime = MPI_Wtime();
        PLOG_INFO << "Sending data to other nodes";
        int rest = matrix_size % numOfNodes;
        int chunk = matrix_size / numOfNodes;
        //int column_count = matrix.at(0).size();
        int column_count = simple_entry.size();
        PLOG_DEBUG << "Rest from division: " << rest;
        for (int currNodeNum = 1; currNodeNum < numOfNodes; currNodeNum++)
        {
            int start = currNodeNum * chunk;
            int end = (currNodeNum + 1) * chunk - 1;
            MPI_Send(&start, 1, MPI_INT, currNodeNum, 0, MPI_COMM_WORLD);
            MPI_Send(&end, 1, MPI_INT, currNodeNum, 0, MPI_COMM_WORLD);
            MPI_Send(&column_count, 1, MPI_INT, currNodeNum, 0, MPI_COMM_WORLD);
            MPI_Send(simple_entry.data(), column_count, MPI_DOUBLE, currNodeNum, 0, MPI_COMM_WORLD);
            

            //MPI_Send(&calculateStruct, 1, mpiCalculateParametersDatatype, i, 0, MPI_COMM_WORLD);
            //MPI_Send(&A[i][0], 3, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);

            
        }
        int local_start = 0;
        int local_end = chunk-1;
        PLOG_DEBUG << "Start: " << local_start << " end: " << local_end << " node number: " << node;
        if (rest != 0) {
            local_start = matrix_size - rest;
            local_end = matrix_size - 1;
            PLOG_DEBUG << "Rest start: " << local_start << " end: " << local_end << " node number: " << node;
        }
        // MPI_Bcast(&B[0][0], 9, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        // // Process node 0 part of parallel processing
        // for (int col = 0; col < output_columns; col++) {
        //     for (int inner = 0; inner < inner_size; inner++) {
        //         PLOG_INFO << "Column: " << col << " inner: " << inner;
        //         C[0][col] += A[0][inner] * B[inner][col];
        //     }
        // }
        // for (int i = 1; i < numOfNodes; i++) {
        //    MPI_Recv(&C[i][0], 3, MPI_DOUBLE,i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // }
    }
    else
    {
        //struct CalculateParameters calcStruct;
        //MPI_Recv(&calcStruct, 1, mpiCalculateParametersDatatype, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        // Processing parallel part for other nodes

        // double B[3][3] = { 0};
        // double LocalA[3] = { 0 };
        // double LocalC[3] = { 0 };
        // B[1][1] = 1.0;
        int local_start = 0;
        int local_end = 0;
        int local_col_count = 0;
        std::vector<std::vector <double> > local_matrix;
        std::vector <double> local_entry;

        //PLOG_INFO << "Calculating integral, node: " << node;
        
        MPI_Recv(&local_start, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&local_end, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&local_col_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        local_entry.resize(local_col_count);
        PLOG_DEBUG << "Local col count: " << local_col_count;
        MPI_Recv(local_entry.data(), local_col_count, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        PLOG_DEBUG << "Start: " << local_start << " end: " << local_end << " node number: " << node;
        PLOG_DEBUG << "Column count: " << local_col_count << " node number: " << node;
        for (double a : local_entry) {
            PLOG_DEBUG << a << " for node number: " << node;
        }
        //MPI_Bcast(&B[0][0], 9, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        //matrix output_matrix(matrix_a.size(), std::vector<double>(matrix_b.at(0).size()));
        // int output_rows = output_matrix.size();
        // int output_columns = output_matrix.at(0).size();
        // int inner_size = matrix_b.size();
        // int output_rows = 1;
        // int output_columns = 3;
        // int inner_size = 3;
        // printf("Multiplying matrixes using sequential method\n");
        // for (int col = 0; col < output_columns; col++) {
        //     for (int inner = 0; inner < inner_size; inner++) {
        //         PLOG_INFO << "Column: " << col << " inner: " << inner;
        //         LocalC[col] += LocalA[inner] * B[inner][col];
        //     }
        // }
        
        // MPI_Send(LocalC, 3, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }
    //PLOG_INFO << "Finished processing, node: " << node;

  
    MPI_Barrier(MPI_COMM_WORLD);
    if (node == 0)
    {
        //Postprocessing
        // endTime = MPI_Wtime();
        // parallelTimeTaken = endTime - startTime;
        // resultStream.str(std::string());
        // resultStream << "Parallized result: " << static_cast<double>(result_quadratic);
        // PLOG_INFO << resultStream.str();
        // resultStream.str(std::string());
        // resultStream << std::setprecision(piPrecision);
        // resultStream << "Parallized Pi result: " << static_cast<double>(result_pi);
        // PLOG_INFO << resultStream.str();
        // PLOG_INFO << "Parallized time: " << parallelTimeTaken << " second(s)";
        // for (int i = 0; i < 3; i++) {
        //     for (int j = 0; j < 3; j++) {
        //         printf("%f, ", C[i][j]);
        //     }
        //     printf("\n");
        // }
    }
    MPI_Finalize();
    return 0;
}