#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "demo.grpc.pb.h"

class DataTransferClient {
public:
    DataTransferClient(std::shared_ptr<grpc::Channel> channel)
            : stub(demo::DataTransfer::NewStub(channel)) {}

    int sendInt(const int value) {
        demo::IntData data;
        data.set_data(value);

        grpc::ClientContext context;
        demo::Response response;

        grpc::Status status = stub->sendInt(&context, data, &response);

        if (!status.ok()) {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            return 400;
        }

        return response.code();
    }

    int sendString(const std::string value) {
        demo::StringData data;
        data.set_data(value);

        grpc::ClientContext context;
        demo::Response response;

        grpc::Status status = stub->sendString(&context, data, &response);

        if (!status.ok()) {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            return 400;
        }

        return response.code();
    }

    int sendFile(const std::string path) {
        static const int bufferSize = 1024;
        char buffer[bufferSize];
        int remainingBytes;
        std::string fileName;

        unsigned long namePos = path.find_last_of('/');
        if(namePos != std::string::npos) {
            fileName = path.substr(namePos + 1);
        } else {
            fileName = path;
        }

        demo::FileData data;
        grpc::ClientContext context;
        demo::Response response;

        std::ifstream file(path, std::ifstream::binary);

        if(file) {
            std::unique_ptr<grpc::ClientWriter<demo::FileData>> writer(stub->sendFile(&context, &response));

            file.seekg (0, file.end);
            remainingBytes = file.tellg();
            file.seekg (0, file.beg);

            data.set_data(fileName);
            if(!writer->Write(data)) {
                std::cout << "I/O error while sending file: " << path << std::endl;
                remainingBytes = 0;
            }

            while(remainingBytes > 0) {
                if(remainingBytes < bufferSize) {
                    file.read(buffer, remainingBytes);
                    data.set_data(buffer, remainingBytes);
                    remainingBytes = 0;
                } else {
                    file.read(buffer, bufferSize);
                    data.set_data(buffer, bufferSize);
                    remainingBytes -= bufferSize;
                }

                if(!writer->Write(data)) {
                    std::cout << "I/O error while sending file: " << path << std::endl;
                }
            }

            writer->WritesDone();
            grpc::Status status = writer->Finish();

            if (!status.ok()) {
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                return 400;
            }

            return response.code();
        } else {
            std::cout << "File doesn't exist!" << std::endl;
            return 400;
        }
    }

private:
    std::unique_ptr<demo::DataTransfer::Stub> stub;
};

int main(int argc, char **argv) {
    std::string option, data;

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " -i INTEGER" << std::endl;
        std::cerr << "Or: " << argv[0] << " -s STRING" << std::endl;
        std::cerr << "Or: " << argv[0] << " -f FILE_PATH" << std::endl;
        return 1;
    }

    DataTransferClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

    for(int i = 1 ; i + 1 < argc ; i += 2) {
        option = argv[i];
        data = argv[i + 1];

        if(strcmp(argv[i], "-i") == 0) {
            int data = atoi(argv[i + 1]);
            client.sendInt(data);
        } else if(strcmp(argv[i], "-s") == 0) {
            client.sendString(argv[i + 1]);
        } else if(strcmp(argv[i], "-f") == 0) {
            client.sendFile(argv[i + 1]);
        } else {
            std::cerr << "Unknown option : " << argv[1] << std::endl;
        }
    }

    return 0;
}
