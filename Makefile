SRCDIR       =  interpret_hse
CXXFLAGS	 =  -O2 -g -Wall -fmessage-length=0 -I../interpret_boolean -I../parse_boolean -I../parse_hse -I../hse -I../boolean -I../parse -I../common
SOURCES	    :=  $(shell find $(SRCDIR) -name '*.cpp')
OBJECTS	    :=  $(SOURCES:%.cpp=%.o)
TARGET		 =  lib$(SRCDIR).a

all: $(TARGET)

$(TARGET): $(OBJECTS)
	ar rvs $(TARGET) $(OBJECTS)

%.o: $(SRCDIR)/%.cpp 
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $<
	
clean:
	rm -f $(OBJECTS) $(TARGET)
