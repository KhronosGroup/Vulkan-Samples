# Vulkan Generators

The Vulkan Headers Repository provides an XML registry which describes the entire API. It also comes with a few helper python packages which can be used to create generators.

The default Vulkan Generator resolves type dependencies in topological order and then passes this information to the generator to populate a file. This means that the generator will only pass you types where their dependant types have been passed previously. To this process in effect you can read the output log of the generator.

## Structure Type Names

It is common for a user to explicitly specify the VkStructureType of a given Vulkan struct. In C this makes complete sense, however, when using C++ we may want to develop interfaces to help generate these structs. Using the C API limits our ability to automatically resolve these names.

`generate_structure_type_names.py` allows for the generation of the `struct_initializers.hpp` file. Here we map struct type definitions to their VkStructureType counter parts. This enables `pNextChain` to automatically populate structs and append them to a chain without user interference.
