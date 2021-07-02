FROM debian:buster

RUN apt-get update \
 && apt-get install --assume-yes libpcre++-dev git python gcc

RUN git clone --depth 1 https://github.com/catboost/catboost.git /catboost
WORKDIR /catboost/catboost/libs/model_interface
RUN ../../../ya make -r .
RUN cp libcatboostmodel.so libcatboostmodel.so.1 /usr/local/lib/
RUN cp c_api.h model_calcer_wrapper.h wrapped_calcer.h /usr/local/include/

WORKDIR /go/src/catboost-memory-leak
COPY main.c model.bin ./

ENV LD_LIBRARY_PATH="/lib:/usr/lib:/usr/local/lib"
RUN gcc -L /usr/local/lib -I /usr/local/include -lcatboostmodel -lpthread -o catboost-memory-leak main.c
ENTRYPOINT ["./catboost-memory-leak"]
