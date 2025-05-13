# Fasthenry GUI
This software adds more user friendly interface to the famous fasthenry-3.0wr package. The fasthenry code which this project is based on is from https://github.com/wrcad/xictools/tree/master/fasthenry version 031424. This project contains the following efforts to help users to use the fasthenry in a easier way than the original linux based codes.

The fasthenry original source code is in the folder "fasthenry-3.0wr-031424", you can put it into a Linux environment to compile and run. The major binary file is fasthenry which can process .inp file containing the geometry and other information of the pattern. The fasthenry program can output Zc.mat file which contains the inductance information. It can also generate zfuffile for the zbuf program (also compiled from the source code) to process to generate .ps file to show the geometry picture of the pattern. The source code also contains the original author's thesis and manual files about how to use the package.

In the test_fasthenry folder, there are the compiled fasthenry and zbuf file, some Mathematic simulation, and a C program to generate .inp file for a micro-SQUID device. In the C code, there is switch to tell fasthenry to calculate normal metal film or the superconducting film inductance. (Refer to Guang Yue PhD thesis, FSU 2017)

The parent folder contains configuration file for VS Code and docker dev container to run the code under windows. Based on this the fasthenry_qt folder contains a Qt GUI code to build docker image and run fasthenry inside a container. The program contains function to verity docker installation, extract the results and show the generated geometry pattern.

## Download the Qt GUI and binary files to use fasthenry
To compile the GUI you need Qt library and the Qt Creator software. You may also download the folder fasthenry_qt/binary_standalone to run the compiled binary file under windows. Put this folder under windows and install docker desktop for windows. Then run the fasthenry_qt.exe.

The GUI contains tips to setup the program and basically you can just clicked the series of buttons from top to bottom to run fasthenry and show results. Each time a new .inp file is loaded, you need to build the docker image and run fasthenry again. The GUI also contains function to show the pattern geometry on the screen. There are two .inp files provided as examples, the squid_normal.inp calculate a micro-SQUID shape of normal metal, the squid_sc.inp calculate the pattern in superconducting state where the kinetic inductance plays more important role.

For more details and more complex usage, please read the source code and make your own changes.

## Use fasthenry under windows through VS Code, docker and dev container
Docker Decktop is a windows software which supports creating isolated evironment to run software inside. By creating such, the docker container, a software can run on different computers with different setup to avoid software compatible issuses and use small system recources. VS Code has extensions to support docker function and the dev container to support developing using docker.

The idea here is to setup a debian container to edit/compile/run the c language based fasthenry code in VS code. The container shares files with the windows system. Then any code changes under windows can be reflected inside the container, and same time any changes of the file in the container will be reflected into the windows folder. Base on this, you may use VS Code under windows to edit the code then let the dev container to compile and run it before the results are returned to windows file system. Effectively you are running the linux C code under windows.

### The steps to setup the dev container with VS Code

- Install Docker Desktop under windows and run it.
- Install VS code, and the extentions of dev containers
- Prepare the source code in the computer (ex. use github desktop app). This repository contains the fasthenry-3.0wr-031424 code.
- create .devcontainer folder in the repository folder. Then create the devcontainer.json and the dockerfile. The repository already contains everything set up. (You may try to use VS Code to create this automatically, see the manual of dev container extension.)

See the devcontainer.json and dockerfile for details. This project just creates a very basic debian dev container and tell docker to install some packages under debian.

### The steps to compile and run fasthenry
- prepare the above dev container or simply fork all the code to your computer from github.
- use VS code to open the code folder as workspace.
- automaticlaly VS code will read the devcontainer.json and the dockerfile to create the dev container image then run the dev container.
- then in the terminal window under VS code, you may enter Linux bash command to control the dev container. All the code files are also seen under the dev container.
- under the terminal
    - go to fasthenry-3.0wr-03144 folder
    - make all
    - you will see the fasthenry programs under the bin folder after compilation.
- go to the test_fasthenry folder which contains a example, execuate the run file which contains the following:
    ```
    !#/bin/bash
    gcc squid.c
    ./a.out
    ../../fasthenry-3.0wr-031424/bin/fasthenry squid_0.inp
    cat Zc.mat
    ```
    This basically compile and run a C program to generate the .inp file for fasthenry to process, i.e. to define the 2D model of a Superconducting QUantum Interferencing Device (SQUID) then calculate the inductance. The Zc.mat file contains the result, you may compare it with the reference file Zc_ref_Guang_Thesis.mat. This example comes from Guang Yue's PhD thesis, Florida State University, 2017.

    For more details of how to use fasthenry see the manual files under the source code folder.

Now you have done of running the fasthenry code under windows. Based on this framework, you may prepare the .inp file using other tools (Python etc) then put the file under the code folder. Then use the dev container/VS Code to run fasthenry to get the results.

