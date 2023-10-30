PID=$(cat /tmp/ex1.pid)
MEM=$(cat /proc/"$PID"/maps)

HEAP_LOC=$(echo "${MEM%%heap*}" | tail -n 1)
HEAP_ADDR="${HEAP_LOC%% *}"
HEAP_BEGIN="0x${HEAP_ADDR%%-*}"
HEAP_END="0x${HEAP_ADDR#*-}"

(sudo gdb gcore "$PID" -ex "find $HEAP_BEGIN, $HEAP_END, 'p', 'a', 's', 's', ':'" -batch) > 'password_addr.txt'
PASSWORD_OUT=$(cat password_addr.txt)
PASSWORD_LOC=${PASSWORD_OUT#*main ()}
PASSWORD_ADDRS=${PASSWORD_LOC%% *}
PASSWORD_ADDR=$(echo $PASSWORD_ADDRS | cut -d ' ' -f1)

(sudo gdb gcore "$PID" -ex "print (char*) $PASSWORD_ADDR" -batch) > 'password.txt'
PASSWORD_RES_OUT=$(cat password.txt)
PASSWORD_RES=${PASSWORD_RES_OUT#*pass:}
echo "PASSWORD:${PASSWORD_RES:0:8}"

rm 'password_addr.txt'
rm 'password.txt'

kill -SIGKILL "$PID"