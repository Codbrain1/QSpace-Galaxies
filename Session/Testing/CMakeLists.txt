qspace_add_test(TestReflectionGadgetToVariantMap QSpace::Session QSpace::Common)
qspace_add_test(TestReflectionObjectSerialize QSpace::Session QSpace::Common)

target_sources(TestReflectionGadgetToVariantMap PRIVATE
    TestGadgets.h
)

target_sources(TestReflectionObjectSerialize PRIVATE
    TestQObjects.h
)