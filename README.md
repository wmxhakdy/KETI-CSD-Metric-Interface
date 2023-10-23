## Introduction of KETI-CSD-Metric-Interface
-------------
KETI-CSD-Metric-Collector for KETI-OpenCSD Platform

communication interface for CSD's metric between CSD and KETI-OpenCSD Storage Node

Developed by KETI

## Contents
-------------
[1. Requirement](#requirement)

[2. How To Install](#How-To-Install)

[3. Governance](#governance)

## Requirement
-------------
>   Ubuntu 18.04.6 LTS

>   RapidJSON

>   CMAKE version 3.26.3

## How To Install
-------------
```bash
git clone 
cd KETI-CSD-Metric-Interface
cd csd-metric-collector/cmake/build
cmake ../../
make -j

#if you fail to build with CMAKE, do this
`export MY_INSTALL_DIR=$HOME/.local`
`export PATH="$MY_INSTALL_DIR/bin:$PATH"`
```

## Governance
-------------
This work was supported by Institute of Information & communications Technology Planning & Evaluation (IITP) grant funded by the Korea government(MSIT) (No.2021-0-00862, Development of DBMS storage engine technology to minimize massive data movement)
