#include <iostream>
#include <mpi.h>
#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <vector>
#include <fstream>
#include <cstdlib>

double A, B, C;

struct CalculateParameters
{
    double A;
    double B;
    double C;
    int N_quadratic;
    double start_point;
    double end_point;
    int N_Pi;
};

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
    int piPrecision = 20;
    int N_quadratic, N_Pi;
    const int struct_size = 7;
    double sum_quadratic, sum_pi, dx, dt, start_point, end_point, result_quadratic, result_pi;
    double startTime, endTime, parallelTimeTaken, timeSingle;
    plog::RollingFileAppender<plog::CsvFormatter> fileAppender("Datalogger.txt", 1048576, 5);
    plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::info, &fileAppender).addAppender(&consoleAppender);
    int node, numOfNodes;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &numOfNodes);
    MPI_Comm_rank(MPI_COMM_WORLD, &node);
    // Change datatype
    int lengths[struct_size] = {1, 1, 1, 1, 1, 1, 1};
    MPI_Datatype types[struct_size] = {MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_INTEGER, MPI_DOUBLE, MPI_DOUBLE, MPI_INTEGER};
    MPI_Aint displacements[struct_size] = {
        offsetof(CalculateParameters, A),
        offsetof(CalculateParameters, B),
        offsetof(CalculateParameters, C),
        offsetof(CalculateParameters, N_quadratic),
        offsetof(CalculateParameters, start_point),
        offsetof(CalculateParameters, end_point),
        offsetof(CalculateParameters, N_Pi)
        };
    
    MPI_Datatype mpiCalculateParametersDatatype;
    MPI_Type_create_struct(struct_size, lengths, displacements, types, &mpiCalculateParametersDatatype);
    MPI_Type_commit(&mpiCalculateParametersDatatype);
    // End of creating datatype

    std::stringstream resultStream;
    resultStream << std::fixed << std::setprecision(standardPrecision);
    if (node == 0)
    {
        PLOG_INFO << "Getting input values";
        //get input parameters
        

        startTime = MPI_Wtime();
        // Start processing on node 0 sequentially
        
        endTime = MPI_Wtime();
        timeSingle = endTime - startTime;
        // TODO rename this string
        resultStream << "Single node integral result: " << static_cast<double>(sum_quadratic);
        PLOG_INFO << resultStream.str();
        PLOG_INFO << "Single node time: " << timeSingle << " second(s)";
        resultStream << std::setprecision(standardPrecision);
        struct CalculateParameters calculateStruct;
        // prepare structure
        startTime = MPI_Wtime();
        calculateStruct.A = A;
        calculateStruct.B = B;
        calculateStruct.C = C;
        calculateStruct.N_quadratic = N_quadratic;
        calculateStruct.start_point = start_point;
        calculateStruct.end_point = end_point;
        calculateStruct.N_Pi = N_Pi;
        PLOG_INFO << "Sending data to other nodes";
        for (int i = 1; i < numOfNodes; i++)
        {
            MPI_Send(&calculateStruct, 1, mpiCalculateParametersDatatype, i, 0, MPI_COMM_WORLD);
            
        }
        // Process node 0 part of parallel processing
    }
    else
    {
        struct CalculateParameters calcStruct;
        MPI_Recv(&calcStruct, 1, mpiCalculateParametersDatatype, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        // Processing parallel part for other nodes
        PLOG_INFO << "Calculating integral, node: " << node;
        
        A = calcStruct.A;
        B = calcStruct.B;
        C = calcStruct.C;
        N_quadratic = calcStruct.N_quadratic;
        N_Pi = calcStruct.N_Pi;
        start_point = calcStruct.start_point;
        end_point = calcStruct.end_point;
    }
    PLOG_INFO << "Finished processing, node: " << node;

    MPI_Reduce(&sum_quadratic, &result_quadratic, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&sum_pi, &result_pi, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    
    MPI_Barrier(MPI_COMM_WORLD);
    if (node == 0)
    {
        //Postprocessing
        endTime = MPI_Wtime();
        parallelTimeTaken = endTime - startTime;
        resultStream.str(std::string());
        resultStream << "Parallized result: " << static_cast<double>(result_quadratic);
        PLOG_INFO << resultStream.str();
        resultStream.str(std::string());
        resultStream << std::setprecision(piPrecision);
        resultStream << "Parallized Pi result: " << static_cast<double>(result_pi);
        PLOG_INFO << resultStream.str();
        PLOG_INFO << "Parallized time: " << parallelTimeTaken << " second(s)";
    }
    MPI_Type_free(&mpiCalculateParametersDatatype);
    MPI_Finalize();
    return 0;
}