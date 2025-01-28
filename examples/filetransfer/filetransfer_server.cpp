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
#include <thread>
#include <memory>

class FileTransferServiceImpl final : public filetransfer::FileTransfer::Service {
public:
  grpc::Status UploadFile(grpc::ServerContext* context,
                          grpc::ServerReader<filetransfer::FileChunk>* reader,
                          filetransfer::UploadStatus* response) override {
    filetransfer::FileChunk chunk;
    std::ofstream output_file;
    std::string filename;
    
    // Read the file chunks from the client
    std::cout << "Uploading file..." << std::endl;
    while (reader->Read(&chunk)) {
      if (!output_file.is_open()) {
        // Open the file for writing the first time
        output_file.open(chunk.filename(), std::ios::binary | std::ios::out);
        if (!output_file) {
          response->set_success(false);
          response->set_message("Failed to open file: " + chunk.filename());
          return grpc::Status::OK;
        }
        else if (filename == "") {
          filename = chunk.filename();
          std::cout << "Began writing file " + filename + "..." << std::endl;
        }
      }

      // Write the chunk content to the file
      output_file.write(chunk.content().data(), chunk.content().size());
    }

    output_file.close();
    response->set_success(true);
    response->set_message("File uploaded successfully.");
    std::cout << "File uploaded: " + filename << std::endl;
    return grpc::Status::OK;
  }

  // DownloadFile implementation
  grpc::Status DownloadFile(grpc::ServerContext* context,
                            const filetransfer::FileProperties* request,
                            grpc::ServerWriter<filetransfer::FileChunk>* writer) override {
      const std::string filename = request->filename();
      const std::int64_t blocksize = request->blocksize();
      std::ifstream input_file(filename, std::ios::binary | std::ios::in);

      // Check if the file can be opened
      if (!input_file) {
          return grpc::Status(grpc::StatusCode::NOT_FOUND, "File not found: " + filename);
      }
      
      // Use a heap-allocated buffer to avoid stack overflow
      const size_t buffer_size = blocksize * 1024; // blocksize KB
      std::unique_ptr<char[]> buffer(new char[buffer_size]);
      filetransfer::FileChunk chunk;

      std::cout << "File found: " << filename << " enqueued in download stream (" << blocksize << "KB block_size)..." << std::endl;

      // Read file in chunks and send to the client
      while (input_file.read(buffer.get(), buffer_size) || input_file.gcount() > 0) {
          chunk.set_filename(filename);
          chunk.set_content(buffer.get(), input_file.gcount());
          if (!writer->Write(chunk)) {
              return grpc::Status(grpc::StatusCode::UNKNOWN, "Failed to write chunk to client.");
          }
      }

      input_file.close();
      std::cout << "File download completed: " << filename << std::endl;
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
  try {
    RunServer();
  } catch (std::exception const& ex) {
    std::cerr << "Server exception caught: " << ex.what() << std::endl;
    return 1;
  }
  return 0;
}