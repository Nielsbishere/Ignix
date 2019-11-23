# Custom igx formats

To support additional info in textures, this format has been created. Every format has to be laid out the following way:

- Start with header
  - char8[4] formatName
  - uint32 versionId
  - Format-dependent data; can include flags of a specific size
- Data whose length can be determined with the header

