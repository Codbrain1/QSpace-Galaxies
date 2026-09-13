qspace_add_test(TestReflectionGadgetToVariantMap QSpace::Session QSpace::Common)
qspace_add_test(TestReflectionObjectSeralize QSpace::Session QSpace::Common)

target_sources(TestReflectionGadgetToVariantMap PRIVATE
    TestGadgets.h
)

target_sources(TestReflectionObjectSeralize PRIVATE
    TestQObjects.h
)