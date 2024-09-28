/// \title TerrnBatcher - tool to merge static meshes & materials for FPS boost
/// \brief Prototype, displays UI, implements selection but not the batching yet.
///
/// DEV NOTES: 
/// -Started as copypaste of 'example_ogre_inspector.as' because I needed the scene traversal logic+UI
/// -Added crude material info display --> to be polished and merged back to Inspector.
/// -Implemented a 'Basket' (selection) mechanism containing 'Picks' (scene nodes)
/// -------------------------------------------------------------------------------------

// #=====================================================================
// #              #### WARNING - DANGEROUS API ####
// #  OGRE objects don't use reference counting
// #    - __ N E V E R __ keep pointers across `frameStep()` invocations!
// #  Prefer always looking up the resource from OGRE - slower but safer.
// #=====================================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

TerrnBatcherUI       tbUI;

void main()
{
    game.log("TERRN BATCHER [ALPHA] - shows how you can traverse scene and merge meshes/textures together for better FPS");
}

void frameStep(float dt)
{
    if (ImGui::Begin("TERRN BATCHER [ALPHA]", closeBtnHandler.draw(), ImGuiWindowFlags_AlwaysAutoResize))
    {
        closeBtnHandler.draw();
        tbUI.draw();
        ImGui::End();
    }
}

class TerrnBatcherUI // Based on Inspector UI, can pick scenenodes and prepare Schedule for the batching.
{
    // dictionary<string, array<uint>> tbuiSelection; ~ Used by the Pick buttons.
    // * Keeping scenenode pointers is dangerous - they aren't refcounted.
    // * We keep a dictionary "scenenode name" -> child node indices
    dictionary tbuiSelection;
    
    // Checkboxes in 'batch controls'
    bool tbuiHideOriginal = true;
    bool tbuiShowNew = true;
    
    // Checkboxes in 'inspector'
    bool tbuiShowDumpButton = false;
    
    // Counters
    uint tbuiNumBatchesCreated = 0;
    uint tbuiNumMeshesDumped = 0;
    
    // Constants
    string tbuiOutputsNodeName = "TerrnBatcher outputs";
    
    //#region Draw UI - main
    void draw()
    {
        
        ImGui::Text("shows how you can traverse scene and merge meshes/textures together for better FPS");
        ImGui::Text("Note that all element names are just simple strings, even if they contain '/'.");
        
        TerrainClass@ terrain = game.getTerrain();
        if (@terrain == null)
        {
            ImGui::TextDisabled("ERROR No terrain loaded");
            return;
        }
        
        Ogre::Root@ root = Ogre::Root::getSingleton();
        if (@root == null)
        {
            ImGui::TextDisabled("ERROR Cannot retrieve OGRE `Root` object");
            return;
        }
        
        
        Ogre::SceneManager@ sceneMgr = this.findSceneManager(root, "main_scene_manager");
        if (@sceneMgr == null)
        {
            ImGui::TextDisabled("ERROR Cannot retrieve scene manager");
            return;
        }
        
        Ogre::SceneNode@ rootNode = sceneMgr.getRootSceneNode();
        if (@rootNode == null)
        {
            ImGui::TextDisabled("ERROR Cannot retrieve root scene node");
            return;
        }     
        
        Ogre::SceneNode@ terrnNode = this.findChildSceneNode(rootNode, "Terrain: " + terrain.getTerrainName());
        if (@terrnNode==null)
        {
            ImGui::TextDisabled("ERROR Cannot retrieve terrain's grouping scene node");
            return;
        }
        
        Ogre::SceneNode@ outputsNode = this.findChildSceneNode(terrnNode, tbuiOutputsNodeName);
        if (@outputsNode==null)
        {
            terrnNode.createChildSceneNode(tbuiOutputsNodeName);
        }
        
        ImGui::Separator();
        ImGui::TextDisabled(" S E L E C T I O N :");
        this.drawSelectionBasket(terrnNode);
        this.drawSelectionBatchControls(terrnNode);
        
        ImGui::Separator();
        ImGui::TextDisabled(" S C E N E   G R A P H :");
        ImGui::Checkbox("Enable mesh dumping (write .mesh file to logs directory)", tbuiShowDumpButton);
        this.drawTreeNodeOgreSceneNodeRecursive(terrnNode);
    }
    //#endregion Draw UI - main
    
    //#region Draw UI - schedule
    void drawSelectionBatchControls(Ogre::SceneNode@ terrnNode)
    {
        if (ImGui::Button(" > > >    B A T C H    < < < "))
        {
            this.batchSelectedMeshes(terrnNode);
        }
        ImGui::SameLine();
        ImGui::Checkbox("hide origial", /*[inout]*/ tbuiHideOriginal);
        ImGui::SameLine();
        ImGui::Checkbox("show new", /*[inout]*/ tbuiShowNew);
    }
    
    void drawSelectionBasket(Ogre::SceneNode@ terrnNode)
    {
        array<string> tobjNodes = tbuiSelection.getKeys();
        
        if (tobjNodes.length() == 0)
        {
            ImGui::Text("nothing selected. Use 'pick' buttons in tree below.");
            return;
        }
        
        // totals
        dictionary materialSet; // material name => int numHits (-1 means not trivial - not supported)
        
    dictionary uniqueSubmeshSet; // dictionary { mesh name + ':' + submesh index => uint numVerts }
    dictionary uniqueSubmeshSetHits; // dictionary { mesh name + ':' + submesh index => int numHits } 
        
        
        for (uint i=0; i<tobjNodes.length(); i++)
        {
            ImGui::Text("\"" +tobjNodes[i]+ "\" --> ");
            
            Ogre::SceneNode@ tobjNode = this.findChildSceneNode(terrnNode,tobjNodes[i]);
            if (@tobjNode == null)
            {
                ImGui::SameLine();
                ImGui::Text("ERROR - not found in scene graph!");
            }
            else
            {
                ImGui::SameLine();
                array<uint>@ nodeIndices = cast<array<uint>>(tbuiSelection[tobjNodes[i]]);
                if (@nodeIndices == null)
                {
                    ImGui::Text("ERROR cast failed");
                }
                else
                {
                    ImGui::Text(nodeIndices.length() + " nodes selected");
                    
                    for (uint j=0; j<nodeIndices.length(); j++)
                    {
                        this.basketProcessSingleInput(materialSet, uniqueSubmeshSet, uniqueSubmeshSetHits, tobjNode, j);
                    }
                }
            }
        }
        
        // now traverse materialSet
        vector2 totalTexSize(0);
        array<string> materialKeys = materialSet.getKeys();
        TerrainClass@ terrain = game.getTerrain();
        for (uint i = 0; i<materialKeys.length(); i++)
        {
            ImGui::BulletText(materialKeys[i]);
            
            Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(materialKeys[i], terrain.getTerrainFileResourceGroup());
            int matNumHits = int(materialSet[materialKeys[i]]);
            bool matTrivialCase = matNumHits != -1;
            if (mat.isNull())
            {
                ImGui::SameLine();
                ImGui::Text("ERROR-could not find material in MaterialManager");
            }
            else if (matTrivialCase)
            {
                Ogre::TexturePtr tex = this.getTrivialCaseMaterialTexture(mat);
                ImGui::SameLine();
                ImGui::Text(" --> " + tex.getName() + " ("+tex.getWidth()+" x " + tex.getHeight() + ") ["+matNumHits+" hits]");
            }
            else
            {
                ImGui::SameLine();
                ImGui::Text("TBD: non trivial materials are not supported yet");
            }
        }
        
        // now traverse  set of unique submeshes
        ImGui::TextDisabled("(by submesh):");
        array<string> submeshKeys = uniqueSubmeshSet.getKeys();
        for (uint i = 0; i<submeshKeys.length(); i++)
        {
            uint numVerts = uint(uniqueSubmeshSet[submeshKeys[i]]);
            ImGui::BulletText(submeshKeys[i] + " --> "+numVerts+" verts ["+uint(uniqueSubmeshSetHits[submeshKeys[i]])+" hits]");
        }
    }
    
    // helper for `drawSelectionBasket()`
    void basketProcessSingleInput(dictionary&inout materialSet, dictionary&inout subMeshSet, dictionary&inout subMeshSetHits, Ogre::SceneNode@ tobjNode, int childIndex)
    {
        Ogre::ChildNodeArray@ children = tobjNode.getChildren();
        array<uint>@ nodeIndices = cast<array<uint>>(tbuiSelection[tobjNode.getName()]);        
        if (nodeIndices[childIndex] >= children.length())
        {
            ImGui::Text("ERROR: '"+tobjNode.getName()+"' has only "+children.length()+"children, requested index"+childIndex);
            return;
        }
        
        // traverse Entities/subentities and collect materials.
        Ogre::SceneNode@ pickedNode = cast<Ogre::SceneNode>(children[nodeIndices[childIndex]]);
        Ogre::MovableObjectArray@ movables = pickedNode.getAttachedObjects();
        if (movables.length() == 1 && movables[0].getMovableType() == "ManualObject")
        {
            // Trivial case - single attached manual object (probably previous terrnbatcher result)
            ImGui::Text("ManualObject="+movables[0].getName());
        }
        else if (movables.length() == 1 && movables[0].getMovableType() == "Entity")
        {
            //  trivial case - single attached entity
            Ogre::Entity@ ent = cast<Ogre::Entity>(movables[0]);
            Ogre::SubEntityArray@ subEntities = ent.getSubEntities();
            for (uint k=0; k<subEntities.length(); k++)
            {
                Ogre::MaterialPtr mat = subEntities[k].getMaterial();
                if (mat.isNull())
                {
                    game.log("ERROR - material is NULL");
                }
                else
                {
                    materialSet[mat.getName()] = (this.isMaterialTrivialCase(mat)) ? int(materialSet[mat.getName()])+1 : -1; 
                    
                    // submesh set
                    Ogre::MeshPtr mesh = ent.getMesh();
                    string submeshKey = mesh.getName() + ':' + k;
                    
                    for (uint l = 0; l < mesh.getSubMeshes().length(); l++)
                    {
                        
                        Ogre::SubMesh@ submesh = mesh.getSubMeshes()[l];
                        uint numVerts = submesh.__getVertexPositions().length();
                        subMeshSet[submeshKey] = numVerts;
                        subMeshSetHits[submeshKey] =  int(subMeshSetHits[submeshKey] )+1;
                    }
                    
                }
                
            }  // END for (subEntities)
        } // END if (single attached entity)
        
    }
    //#endregion Draw UI - schedule
    
    // #region Pick helpers
    
    void pickSceneNode(string key, uint index)
    {
        if (this.isSceneNodePicked(key, index))
        {
            game.log("DBG pickSceneNode(): this is already picked: "+key+" --> "+index);
            return;
        }
        if (!tbuiSelection.exists(key))
        {
            tbuiSelection[key] = array<uint>();
        }
        array<uint>@ arr = cast<array<uint>>(tbuiSelection[key]);
        arr.insertLast(index);
        game.log("DBG picked "+key+" --> "+index);
    }
    
    void unPickSceneNode(string key, uint index)
    {
        if (!this.isSceneNodePicked(key, index))
        {
            game.log("DBG unPickSceneNode(): this is not picked: "+key+" --> "+index);
            return;
        }
        array<uint>@ arr = cast<array<uint>>(tbuiSelection[key]);
        for (uint i=0; i<arr.length(); i++)
        {
            if (arr[i]==index) 
            { 
                arr.removeAt(i); 
                game.log("DBG UN-picked "+key+" --> "+index);
                return; 
            }
        }   
    }
    
    bool isSceneNodePicked(string key, uint index)
    {
    if (!tbuiSelection.exists(key)) { return false; }
        array<uint>@ arr = cast<array<uint>>(tbuiSelection[key]);
        for (uint i=0; i<arr.length(); i++)
        {
        if (arr[i]==index) { return true; }
        }
        return false;
    }
    
    // #endregion Pick helpers
    
    //#region Searching helpers
    
    Ogre::SceneManager@ findSceneManager(Ogre::Root@ root, string subject)
    {
        Ogre::SceneManagerInstanceDict@ sceneManagers = root.getSceneManagers();
        
        array<string> sceneMgrNames = sceneManagers.getKeys();
        for (uint i = 0; i < sceneManagers.getSize(); i++)
        {
            if (sceneMgrNames[i] == subject)
            {
                return sceneManagers[sceneMgrNames[i]];
            }       
        }
        return null;
    }
    
    Ogre::SceneNode@ findChildSceneNode(Ogre::SceneNode@ snode, string subject)
    {
        Ogre::ChildNodeArray@ children = snode.getChildren();
        for (uint i = 0; i < children.length(); i++)
        {
            Ogre::SceneNode@ child = cast<Ogre::SceneNode>(children[i]);
            if (@child != null && child.getName() == subject)
            {
                return child;
            }
        }
        return null;
    }
    
    //#endregion Searching helpers
    
    //#region Inspector tree nodes
    void drawTreeNodeOgreSceneNodeRecursive(Ogre::SceneNode@ snode, uint indexUnderParent=0)
    {
        // Start with all nodes folded (root node can have hundreds...)
        ImGui::SetNextItemOpen(false, ImGuiCond_Once);
        
        Ogre::ChildNodeArray@ children = snode.getChildren();
        Ogre::MovableObjectArray@ movables = snode.getAttachedObjects();
        
        // The `__getUniqueName()` is a Rigs of Rods extension (that's why double leading underscores), 
        // because names are optional and usually not set, and imgui tree nodes require unique IDs.
        ImGui::PushID(snode.__getUniqueName());
        string treeNodeCaption = snode.getName();
        if (treeNodeCaption == "")
        {
            treeNodeCaption = "["+(indexUnderParent+1)+"]";
        }
        
        if (ImGui::TreeNode(treeNodeCaption))
        {
            // Tree node open, draw children recursively
            ImGui::TextDisabled("Ogre::Node ["+children.length()+"]");
            for (uint i = 0; i < children.length(); i++)
            {
                Ogre::SceneNode@ child = cast<Ogre::SceneNode>(children[i]);
                if (@child != null)
                {
                    drawTreeNodeOgreSceneNodeRecursive(child, i);
                }
            }
            
            // Draw attached movable objects
            ImGui::TextDisabled("Ogre::MovableObject [" + movables.length() + "]");
            for (uint i = 0; i < movables.length(); i++)
            {
                drawTreeNodeOgreMovableObject(movables[i]);
            }
            
            ImGui::TreePop();
        }
        else
        {
            
            
            // Tree node closed, draw info (context-sensitive)
            ImGui::SameLine();
            if (children.length() == 0 && movables.length() == 1)
            {
                this. drawInlineSceneNodeControlsToInspector(snode, indexUnderParent);
            }
            else 
            {
                ImGui::Text("("+children.length()+" children, "+movables.length()+" movables)");
            }
            
            
        }
        ImGui::PopID(); //snode.__getUniqueName()
    }
    
    void drawTreeNodeOgreMovableObject(Ogre::MovableObject@ movable)
    {
        if (ImGui::TreeNode(movable.__getUniqueName()))
        {
            bool visible = movable.isVisible();
            if (ImGui::Checkbox("Visible", visible))
            {
                movable.setVisible(visible);
            }
            
            bool castShadows = movable.getCastShadows();
            if (ImGui::Checkbox("Cast shadows", castShadows))
            {
                movable.setCastShadows(castShadows);
            }
            
            if (movable.getMovableType() == "Entity")
            {
                Ogre::Entity@ entity = cast<Ogre::Entity>(movable);
                Ogre::AnimationStateSet@ stateSet = entity.getAllAnimationStates();
                if (@stateSet != null)
                {
                    Ogre::AnimationStateDict@ stateDict = stateSet.getAnimationStates(); 
                    ImGui::TextDisabled("Ogre::AnimationState [" + stateDict.getSize() + "]");
                    array<string> stateNames = stateDict.getKeys();
                    for (uint i = 0; i < stateDict.getSize(); i++)
                    {
                        this.drawTreeNodeOgreAnimationState(stateDict[stateNames[i]]);
                    }
                }
                
                // +TerrnBatcher - subEntities
                Ogre::SubEntityArray@ subEntities = entity.getSubEntities();
                ImGui::TextDisabled("Ogre::SubEntity [" + subEntities.length() + "]");
                for (uint i = 0; i < subEntities.length(); i++)
                {
                    this.drawTreeNodeOgreSubEntity(subEntities, i);
                }
                // END +TerrnBatcher
            }
            
            ImGui::TreePop();
        }
    }
    
    void drawTreeNodeOgreAnimationState(Ogre::AnimationState@ anim)
    {
        ImGui::BulletText('"' + anim.getAnimationName() + '"');
        ImGui::SameLine();
        ImGui::Text("(" + formatFloat(anim.getLength(), "", 0, 2) + " sec)");
        if (anim.getEnabled())
        {
            ImGui::SameLine();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, vector2(0.f, 0.f));
            ImGui::ProgressBar(anim.getTimePosition() / anim.getLength());
            ImGui::PopStyleVar(1); // ImGuiStyleVar_FramePadding
        }
    }
    //#endregion Inspector tree nodes
    
    // #region TerrnBatcher controls for inspector
    
    void drawInlineSceneNodeControlsToInspector(Ogre::SceneNode@ snode, uint indexUnderParent)
    {
        //the most usual case - a node with a single entity.  Draw name and controls inline
        // ------------------------------------------------
        Ogre::MovableObjectArray@ movables = snode.getAttachedObjects();
        ImGui::TextDisabled("-->");
        ImGui::SameLine();
        ImGui::Text(movables[0].getName());
        ImGui::SameLine();
        
        // Inspector: go to
        if (ImGui::SmallButton("go to")) 
        {
            game.setPersonPosition(snode.getPosition());
        }                
        
        // +TerrnBatcher - visibility
        ImGui::SameLine();
        bool visible = movables[0].isVisible();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, vector2(2,0));
        if (ImGui::Checkbox("Visible", visible))
        {
            movables[0].setVisible(visible);
        }
        ImGui::PopStyleVar(); //ImGuiStyleVar_FramePadding
        
        // +terrnBatcher - selection
        ImGui::SameLine();
        if (this.isSceneNodePicked(snode.getParentSceneNode().getName(), indexUnderParent))
        {
            if(ImGui::SmallButton("-UnPick"))
            {
                this.unPickSceneNode(snode.getParentSceneNode().getName(), indexUnderParent);
            }
        }
        else
        {
            if (ImGui::SmallButton("+Pick!")) 
            {
                
                this.pickSceneNode(snode.getParentSceneNode().getName(), indexUnderParent);
            }
        }
        
        // +TerrnBatcher - mesh serialization
        if (tbuiShowDumpButton)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Dump"))
            {
                if (this.dumpMesh(movables[0]))
                {
                    game.log("TerrnBatcher: Mesh dumped successfully");
                }
            }
        }
    }
    
    bool dumpMesh(Ogre::MovableObject@ movable)
    {
        // Save to 'logs' directory - Make sure filenames dont clash
        // ----------------------------------------------------------
        string fileName = "TerrnBatcher(NID" + thisScript + ")_Dump" + (tbuiNumMeshesDumped++) + "_";
        Ogre::MeshPtr mesh;
        if (movable.getMovableType() == "Entity")
        {
            Ogre::Entity@ ent = cast<Ogre::Entity>(movable);
            mesh = ent.getMesh();
            fileName += mesh.getName();
        }
        else if (movable.getMovableType() == "ManualObject")
        {
            Ogre::ManualObject@ mo = cast<Ogre::ManualObject>(movable);
            mesh = mo.convertToMesh(fileName, "Logs");
            fileName += "batch.mesh";
        }
        else
        {
            game.log("ERROR dumpMesh(): unrecognized MovableObject type '" + movable.getMovableType() + "'");
            return false;
        }
        
        return game.serializeMeshResource(fileName, "Logs", mesh);
    }
    
    // #endregion TerrnBatcher controls for inspector
    
    // #region Material helpers
    
    Ogre::TexturePtr getTrivialCaseMaterialTexture(Ogre::MaterialPtr mat)
    {
        if (mat.isNull()) return Ogre::TexturePtr(); // null
        Ogre::TechniqueArray@ techs = mat.getTechniques();
        if (techs.length() != 1) return Ogre::TexturePtr(); // null
        Ogre::PassArray@ passes = techs[0].getPasses();
        if (passes.length() != 1) return Ogre::TexturePtr(); // null
        Ogre::TextureUnitStateArray@ tuss = passes[0].getTextureUnitStates();
        if (tuss.length() != 1) return Ogre::TexturePtr(); // null
        return tuss[0]._getTexturePtr();
    }
    
    bool isMaterialTrivialCase(Ogre::MaterialPtr mat)
    {
        return !this.getTrivialCaseMaterialTexture(mat).isNull();
    }
    
    // #endregion Material helpers
    
    void drawTreeNodeOgreSubEntity(Ogre::SubEntityArray@ subEntities, uint idx)
    {
        if (subEntities.length() <= idx) return; // sanity check
        
        Ogre::SubEntity@ subEntity = subEntities[idx];
        Ogre::MaterialPtr mat = subEntity.getMaterial();
        if (!mat.isNull())
        {
            Ogre::TechniqueArray@ techs = mat.getTechniques();
            ImGui::Text("|> Material: \"" + mat.getName() + "\" ("+techs.length()+" techniques)");
            for (uint i = 0; i < techs.length(); i++)
            {
                Ogre::PassArray@ passes = techs[i].getPasses();
                ImGui::Text("|> > Technique: \"" + techs[i].getName()+"\" ("+passes.length()+" passes)");
                for (uint j=0; j < passes.length(); j++)
                {
                    Ogre::TextureUnitStateArray@ tuss = passes[j].getTextureUnitStates();
                    ImGui::Text("|> > > Pass: \"" + passes[j].getName() + "\" (" + tuss.length() + " texture units)");
                    for (uint k=0; k<tuss.length(); k++)
                    {
                        ImGui::Text("|> > > > Texture unit: \"" + tuss[k].getName() + "\"");
                        Ogre::TexturePtr tex = tuss[k]._getTexturePtr();
                        if (!tex.isNull())
                        {
                            ImGui::SameLine();
                            ImGui::Text("Texture=\"" + tex.getName()+"\"");
                        }
                    }
                }
            }
        }
        else
        {
            ImGui::Text("no material");
        }
    }
    
    // #region The actual magic - merging meshes
    void addSingleSubMeshToBatch(Ogre::ManualObject@ mo, Ogre::SubMesh@ subMesh, vector3 pos, quaternion rot, vector3 scale)
    {
        // Proof of concept: make it a world-space mesh. Assume tri-list type mesh with  1 UV (=texcoords) layer
        // ------------------------------------------------------------------------
        
        
        uint indexBase = mo.getCurrentVertexCount();
        
        array<vector3> vertPos = subMesh.__getVertexPositions();
        array<vector2> vertUVs = subMesh.__getVertexTexcoords(0);
        for (uint iVert = 0; iVert < vertPos.length(); iVert++)
        {
            mo.position((rot * vertPos[iVert]) * scale + pos);
            mo.textureCoord(vertUVs[iVert]);
        }
        
        if (subMesh.__getIndexType() == Ogre::IndexType::IT_16BIT)
        {
            array<uint16> indexBuf = subMesh.__getIndexBuffer16bit();
            for (uint iIndex = 0; iIndex < indexBuf.length(); iIndex++)
            {
                mo.index(indexBase + indexBuf[iIndex]);
            }
        }
        else
        {
            game.log("ERROR appendMeshInstanceToManualObject(): mesh is not supported - not 16-bit indexed");
        }
    }
    
    void addSceneNodeAttachedMeshesToBatch(Ogre::ManualObject@ mo, Ogre::SceneNode@ pickedNode, string&inout foundMatName)
    {
        Ogre::MovableObjectArray@ movables = pickedNode.getAttachedObjects();
        for (uint iMovas = 0; iMovas < movables.length(); iMovas++)
        {
            //game.log("DBG addSceneNodeAttachedMeshesToBatch(): iMovas="+iMovas+"/"+movables.length());
            if (movables[iMovas].getMovableType() != "Entity")
            {
                game.log("DBG batchSelectedMeshes(): skipping movable of type '"+movables[iMovas].getMovableType()+"' - not suported!");
                continue;
            }
            Ogre::Entity @ent = cast<Ogre::Entity>(movables[iMovas]);
            
            Ogre::SubEntityArray@ subEntities = ent.getSubEntities();
            for (uint iSubent=0; iSubent<subEntities.length(); iSubent++)
            {
                //game.log("DBG addSceneNodeAttachedMeshesToBatch(): iSubent="+iSubent+"/"+subEntities.length());
                Ogre::MaterialPtr mat = subEntities[iSubent].getMaterial();
                if (mat.isNull())
                {
                    game.log("DBG batchSelectedMeshes(): skipping subEntity - material is NULL");
                    continue;
                }
                if (foundMatName == "")
                {
                    foundMatName = mat.getName();
                    mo.begin(foundMatName, Ogre::OT_TRIANGLE_LIST, game.getTerrain().getHandle().getTerrainFileResourceGroup());
                    game.log ("DBG  addSceneNodeAttachedMeshesToBatch(): Manual object initialized with material '"+foundMatName+"'");
                }
                else if (foundMatName != mat.getName())
                {
                    game.log("DBG WARNING batchSelectedMeshes(): ignoring mat '"+mat.getName()+"', already using '"+foundMatName+"'");
                }
                Ogre::SubMesh@ subMesh = subEntities[iSubent].getSubMesh();
                if (@subMesh == null)
                {
                    game.log("DBG ERROR batchSelectedMeshes(): skipping subEntity - submesh is NULL");
                    continue;
                }
                
                this.addSingleSubMeshToBatch(mo, subMesh, pickedNode.getPosition(), pickedNode.getOrientation(), pickedNode.getScale());
            }            
            
        }
    }
    
    void batchSelectedMeshes(Ogre::SceneNode@ terrnNode)
    {
        // proof of concept - only merge meshes, do nothing about the material.
        // Assume full transform (relative to world origin) is equal to local transform (relative to terrain grouping scenenode)
        // ----------------------------------------------------------------------------------------------------------------------
        
        string moName = this.composeUniqueId("batch #" + tbuiNumBatchesCreated++);
        //game.log("DBG batchSelectedMeshes(): moName='"+moName+"'");
        Ogre::ManualObject@ mo = game.getSceneManager().createManualObject(moName);
        string foundMatName = "";
        if (@mo == null)
        {
            game.log("ERROR batchSelectedMeshes(): could not create manual object");
            return;
        }
        
        // Loop through selected 'grouping' nodes (1 for each .tobj / .as) file
        
        array<string> tobjNodes = tbuiSelection.getKeys();
        if (tobjNodes.length() == 0) 
        { 
            game.log("ERROR batchSelectedMeshes(): nothing selected. Use 'pick' buttons in tree below.");
            return;
        }      
        
        game.log("DBG batchSelectedMeshes(): num selected nodes='"+tobjNodes.length()+"'");
        for (uint iNode=0; iNode<tobjNodes.length(); iNode++)
        {
            Ogre::SceneNode@ tobjNode = this.findChildSceneNode(terrnNode, tobjNodes[iNode]);
            if (@tobjNode == null)
            {
                game.log("DBG batchSelectedMeshes(): '"+tobjNodes[iNode]+"' - not found in scene graph!");
                continue;
            }
            
            // Loop through list of scene nodes selected under this grouping node
            array<uint>@ nodeIndices = cast<array<uint>>(tbuiSelection[tobjNodes[iNode]]);
            
            //game.log("DBG batchSelectedMeshes(): processing node "+uint(iNode+1)+"/"+tobjNodes.length()+" ("+tobjNodes[iNode]+") - "+nodeIndices.length()+" nodes picked");
            
            for (uint iChild=0; iChild<nodeIndices.length(); iChild++)
            {
                Ogre::ChildNodeArray@ children = tobjNode.getChildren();
                
                if (nodeIndices[iChild] >= children.length())
                {
                    game.log("ERROR: '"+tobjNode.getName()+"' has only "+children.length()+"children, requested index"+nodeIndices[iChild]);
                    continue;
                }   
                
                // Process the entities attached to this scene node
                Ogre::SceneNode@ childNode = cast<Ogre::SceneNode>(children[nodeIndices[iChild]]);
                
                //game.log("DBG batchSelectedMeshes(): iChild="+iChild+"/"+nodeIndices.length()+", uniqueName()="+childNode.__getUniqueName());
                this.addSceneNodeAttachedMeshesToBatch(mo, childNode, /*[inout]*/foundMatName);
                if (tbuiHideOriginal)
                {
                    childNode.setVisible(false);
                }
            } // END loop (list of picked nodes)
        } // END loop (grouping nodes)
        
        if (foundMatName != "")
        {
            mo.end();
            
            Ogre::SceneNode@ outputsNode = this.findChildSceneNode(terrnNode, tbuiOutputsNodeName);
            Ogre::SceneNode@ snode = outputsNode.createChildSceneNode(moName+"-node");
            snode.attachObject(cast<Ogre::MovableObject@>(mo));
            snode.setVisible(tbuiShowNew);
            // unlike the ManualObject example, we don't position the node - verts are already in world position.     
        }
        else
        {
            game.log("DBG batchSelectedMeshes(): material not found, mesh not generated");
        }
    }
    
    // #endregion The actual magic - merging meshes
    
    // #region Generic helpers
    string composeUniqueId(string name)
    {
        // to avoid clash with leftover scene nodes created before, we include the NID in the name - using automatic global var `thisScript`.
        return "TerrnBatcher(NID:"+thisScript+"): "+name;
    }
    // #endregion Generic helpers    
}

