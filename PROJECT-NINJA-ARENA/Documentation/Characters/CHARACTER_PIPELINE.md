# Character Asset Pipeline

1. Preserve original source asset.
2. Inspect scale, orientation, materials and topology.
3. Create/validate humanoid skeleton and skin weights in Blender.
4. Export FBX with consistent unit scale and skeleton naming.
5. Import as Skeletal Mesh into Unreal.
6. Retarget to the project's shared base skeleton where practical.
7. Validate idle, locomotion, jump, dash, attacks and hit reactions.
8. Profile mesh complexity, materials, texture sizes and LODs.

Static GLB assets cannot be used as animated Skeletal Meshes until rigging and skinning are completed.
