#!/bin/sh
case ${1} in
	-m|--mock)
		./mock_smtp.expect &
		MOCK_PID=$!
		sleep 2
		shift
	;;
esac

SENDMAIL_URL=${1:-http://localhost:8080/}

curl -v -d @test.json -H "Content-Type: application/json" "${SENDMAIL_URL}"

if [ -n ${MOCK_PID} ]
then
	kill ${MOCK_PID} > /dev/null 2>&1
fi
