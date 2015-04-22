FROM centos:centos6
MAINTAINER Richard Guest

ADD epel.repo /etc/yum.repos.d/epel-bootstrap.repo
RUN yum clean expire-cache && \
    yum --disablerepo='*' --enablerepo=epel -y install epel-release && \
    rm /etc/yum.repos.d/epel-bootstrap.repo

RUN yum install -y \
    boost-devel \
    cmake \
    flex \
    gcc-c++ \
    gcc-gfortran \
    git \
    libcdio-devel \
    libxml2-devel \
    mysql-devel \
    ncurses-devel \
    python-devel \
    qt4-devel

WORKDIR /opt/src/seiscomp3/build

CMD cmake ..

