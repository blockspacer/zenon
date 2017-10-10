

CFLAGS = -Wall -std=c++14 -Wno-sign-compare -Wno-unused-variable -Wno-shift-count-overflow -Wno-tautological-constant-out-of-range-compare -Wno-mismatched-tags -ftemplate-depth=512

CC = g++
LD = g++

CFLAGS += -Werror -Wimplicit-fallthrough -g
LDFLAGS += -fuse-ld=lld

OBJDIR = obj

NAME = zygmunt

ROOT = ./
TOROOT = ./../
IPATH = -I.

CFLAGS += $(IPATH)

LDFLAGS += -L/usr/local/lib

SRCS = $(shell ls -t *.cpp)

LIBS = -L/usr/lib/x86_64-linux-gnu  ${LDFLAGS}


OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))
DEPS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.d))
DEPS += $(OBJDIR)/stdafx.h.d

##############################################################################

all:
	@$(MAKE) --no-print-directory info
	@$(MAKE) --no-print-directory compile

compile: $(NAME)

$(OBJDIR)/stdafx.h.gch: stdafx.h
	$(CC) -x c++-header $< -MMD $(CFLAGS) -o $@

PCH = $(OBJDIR)/stdafx.h.gch
PCHINC = -include-pch $(OBJDIR)/stdafx.h.gch

$(OBJDIR)/%.o: %.cpp ${PCH}
	$(CC) -MMD $(CFLAGS) $(PCHINC) -c $< -o $@

$(NAME): $(OBJS)
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS)

info:
	@$(CC) -v 2>&1 | head -n 2

clean:
	$(RM) $(OBJDIR)/*.o
	$(RM) $(OBJDIR)/*.d
	$(RM) $(OBJDIR)/test
	$(RM) $(OBJDIR)-opt/*.o
	$(RM) $(OBJDIR)-opt/*.d
	$(RM) $(NAME)
	$(RM) $(OBJDIR)/stdafx.h.*

-include $(DEPS)
