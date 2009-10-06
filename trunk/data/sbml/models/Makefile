XSD_DIR=../schema
L2V3=$(XSD_DIR)/sbml-l2v3.xsd
L1V2=$(XSD_DIR)/sbml-l1v2.xsd
MODELS=dd me hsr cr bd vilar
L1V2_CHECK=$(addsuffix -l1v2.ok1,$(MODELS))
L2V3_CHECK=$(addsuffix .ok2,$(MODELS))
XFRM21=convert-l2v3-l1v2.xsl

default: check2

check1: $(L1V2_CHECK)

check2: $(L2V3_CHECK) cleanup

%.ok2: %.xml
	@xmllint -o $@ --schema $(L2V3) $<

%.ok1: %.xml
	@xmllint -o $@ --schema $(L1V2) $<

%-l1v2.xml: %.xml $(XFRM21) kineticLaw.xsl
	@xsltproc $(XFRM21) $< | xmllint --format - > $@


cleanup:
	@rm -f *.ok[12]

clean: cleanup
	rm -f *~ *-l1v2.xml


.PHONY: default check1 check2 cleanup clean
