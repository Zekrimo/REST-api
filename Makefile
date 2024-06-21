CFLAGS=-std=c99 -Wall `pkg-config --cflags glib-2.0` -D NDEBUG
LDLIBS=`pkg-config --libs glib-2.0` -lm

# Deze code hoort bij de parseropdracht. Het is een gedeeltelijke
# implementatie die werkt. De codekwaliteit is echter onvoldoende.
#
# Voor het maken van de parseropdracht en de eindopdracht mag (nee, moet)
# je code aan server/ toevoegen. Als gevolg daarvan zal je ook de inhoud
# van deze Makefile moeten uitbreiden. Verdere wijziging is zonder
# overleg niet toegestaan!
#
# Echter, de werking van de Makefile mag niet wijzigen: `make`,
# `make arduino` en `make rpi` moeten blijven doen wat ze nu ook doen,
# namelijk, het uitvoeren van de integratietest respectievelijk
# acceptatietest.
#
# Voor het assessment is het essentieel dat:
#
# 1. de code zonder waarschuwing compileert
# 2. cppcheck geen waarschuwingen geeft
# 3. de integratietest foutloos draait
# 4. de acceptatietests op Arduino en Raspberry foutloos draaien
#
# Bovenstaande vier punten moet zonder aanpassing aan deze Makefile
# ook gelden wanneer de integratietest en acceptatietests vervangen worden.
#
# Tot slot, de eindopdracht moet worden opgeleverd zonder gecompileerde
# bestanden.

# your configuration:

arduino_IP=192.168.1.21
rpi_IP=192.168.1.11

object_files=$(project)/cserver.o buffermock.o

# do not change:

project=arduino_server
unittest=integration_test
arduinotest=acceptation_test_arduino.py
rpitest=acceptation_test_rpi.py
virtualEnvironment = virtEnv

.PHONY: test_c_code arduino rpi fmt check prepare accept_arduino accept_rpi clean

test_c_code: check fmt clean

# needs a running server on Arduino on $(arduino_IP)
arduino: prepare fmt accept_arduino 

# needs a running server on RPi on $(rpi_IP)
rpi: prepare fmt accept_rpi


$(unittest): $(object_files)

fmt:
	clang-format -style='{PointerAlignment: Left, ColumnLimit: 60}' -i $(project)/*.c $(project)/*.h $(project)/*.ino
	clang-format -style='{PointerAlignment: Left, ColumnLimit: 60}' -i *.c *.h
	autopep8 -i *.py

check: $(unittest)
	cppcheck -q --enable=all --inconclusive --std=c99 $(project)/
	./$(unittest)

# bar limits check to availability, no timestamp check performed
prepare: | $(virtualEnvironment)

$(virtualEnvironment):
	python3 -m venv $(virtualEnvironment)
	./$(virtualEnvironment)/bin/pip3 install pytest
	./$(virtualEnvironment)/bin/pip3 install requests
	./$(virtualEnvironment)/bin/pip3 install --force-reinstall -v "requests-raw==1.0.1"

accept_arduino:
	ip=$(arduino_IP) ./$(virtualEnvironment)/bin/python3 $(arduinotest)

accept_rpi:
	ip=$(rpi_IP) ./$(virtualEnvironment)/bin/python3 $(rpitest)

clean:
	-rm *.o $(project)/*.o
	-rm -rf $(unittest).dSYM/
	-rm $(unittest)

refresh:
	-rm -rf __pycache__/
	-rm -rf .pytest_cache/
	-rm -rf $(virtualEnvironment)

