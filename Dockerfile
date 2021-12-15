FROM alpine as build-env

RUN apk add --no-cache build-base
## change this working directory to wherever you have saved the github repository on your local machine 
WORKDIR C:\Users\Luka\Desktop\github\banking-application-c

COPY . .

RUN gcc Assignment3.c -lpthread -o asn3.out

CMD ["./asn3.out"]