#pragma once

void Sigaction(int SIG, void (*handler_func)(int));
void Read(int fd, void* dest, size_t n_bytes);
void Write(int fd, void* src, size_t n_bytes);
void Bind(int sock, short port);
void ReadWrite(int dest_fd, int source_fd);