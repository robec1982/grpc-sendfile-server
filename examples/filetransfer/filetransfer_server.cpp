// Copyright 2019 Carlos O'Ryan
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <filetransfer.grpc.pb.h>
#include <boost/program_options.hpp>
#include <grpcpp/grpcpp.h>
#include <functional>
#include <iostream>
#include <fstream>

class FileTransferServiceImpl final : public filetransfer::FileTransfer::Service {
public:
  grpc::Status UploadFile(grpc::ServerContext* context,
                          grpc::ServerReader<filetransfer::FileChunk>* reader,
                          filetransfer::UploadStatus* response) override {
    filetransfer::FileChunk chunk;
    std::ofstream output_file;

    while (reader->Read(&chunk)) {
      if (!output_file.is_open()) {
        // Open the file for writing the first time
        output_file.open(chunk.filename(), std::ios::binary | std::ios::out);
        if (!output_file) {
          response->set_success(false);
          response->set_message("Failed to open file: " + chunk.filename());
          return grpc::Status::OK;
        }
      }

      // Write the chunk content to the file
      output_file.write(chunk.content().data(), chunk.content().size());
    }

    output_file.close();
    response->set_success(true);
    response->set_message("File uploaded successfully.");
    return grpc::Status::OK;
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  FileTransferServiceImpl service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Filetransfer server listening on " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();
  return 0;
}