FROM mcr.microsoft.com/devcontainers/base:bullseye
RUN apt-get update && apt-get install dos2unix vim -y
RUN mkdir -p /test
ADD fasthenry /test
ADD tmp.inp /test
ADD run.sh /test
RUN dos2unix /test/tmp.inp /test/run.sh







