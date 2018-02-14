#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "demo.grpc.pb.h"

class DataTransferService final : public demo::DataTransfer::Service {
    grpc::Status sendInt(grpc::ServerContext *context, const demo::IntData *data, demo::Response *response) override {
        std::cout << "IntData: " << data->data() << std::endl;
        response->set_code(200);
        return grpc::Status::OK;
    }

    grpc::Status sendString(grpc::ServerContext *context, const demo::StringData *data, demo::Response *response) override {
        std::cout << "StringData: " << data->data() << std::endl;
        response->set_code(200);
        return grpc::Status::OK;
    }

    grpc::Status sendFile(grpc::ServerContext* context, grpc::ServerReader<demo::FileData>* reader, demo::Response* response) override {
        static const std::string basePath = "received/";
        demo::FileData data;
        std::string fileName;

        if(reader->Read(&data)) {
            fileName = basePath + data.data();
        }

        std::ofstream file (fileName, std::ofstream::binary);

        while(reader->Read(&data)) {
            file.write(data.data().data(), data.data().length());
        }

        file.close();

        response->set_code(200);
        std::cout << "FileData: " << fileName << std::endl;

        return grpc::Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    DataTransferService service;

    grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char **argv) {
    RunServer();

    return 0;
}
