FROM alpine as build-env

RUN apk add --no-cache build-base

WORKDIR C:\Users\Luka\Desktop\github\banking-application-c

COPY . .

ENV PORT=8080

EXPOSE 8080

RUN gcc Assignment3.c -lpthread -o asn3.out

CMD ["./asn3.out"]