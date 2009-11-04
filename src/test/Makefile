models=cr dd hsr me
profs=$(addprefix p,$(models))
files=$(models) $(addsuffix -opt.xml,$(models)) $(addsuffix .out,$(models))
files+=$(profs) $(addsuffix .cnt,$(profs)) $(addsuffix .ord,$(profs)) $(addsuffix .xsl,$(profs))

default: test.log

test.log:
	./test.sh $(models) | tee test.log

clean:
	rm -f $(files)
	rm -f test.log *~
