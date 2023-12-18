import {
    AF_INET, SOCK_STREAM, sockaddr_in,
    socket, read, write, connect, close,
    inet_aton, htons
} from "socket";

function main ()
{
    let s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        perrr("create socket failed\n");
        return 1;
    }

    let addr = new sockaddr_in();

    inet_aton("10.8.9.5", addr.sin_addr);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);

    let r = connect(s, addr, sockaddr_in.size);
    if (r == -1) {
        prerr("connect failed\n");
        return 1;
    }

    print("connected!\n");

    let ab = new ArrayBuffer(1024);

    let len = encodeText("GET / HTTP:1.1\r\n\r\n", null, ab);

    r = write(s, ab, len);
    if (r != len) {
        prerr("write failed\n");
        return 1;
    }

    while (true) {

        r = read(s, ab, ab.byteLength);
        if (r == -1) {
            prerr("read failed\n");
            return 1;
        }

        if (r == 0)
            break;

        print(`read ${r} bytes\n`);
        let str = decodeText(null, ab);
        print(str, "\n");
    }

    close(s);
}