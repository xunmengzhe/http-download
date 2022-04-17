PROJ := dl
SOURCE :=  ${wildcard *.cc}
OBJS := ${patsubst %.cc, %.o, $(SOURCE)}
LIBS := -lcurl -lpthread
CXXFLAGS += -Wall -DDEBUG
CXX := @g++
RM := @rm -fR

.PHONY: all clean

all: $(PROJ)

$(PROJ):
	@echo "-------- [$@] compile starting ...... --------"
	$(CXX) -c $(SOURCE) $(CXXFLAGS)
	$(CXX) -o $(PROJ) $(OBJS) $(LIBS)


clean:
	@echo "######## Clean $(PROJ) starting ...... ########"
	$(RM) $(OBJS) $(PROJ)
