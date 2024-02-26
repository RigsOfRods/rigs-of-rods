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

#include "imgui_utils.as"

TerrnBatcherUI       tbUI;

void main()
{
    game.log("TERRN BATCHER [ALPHA] - shows how you can traverse scene and merge meshes/textures together for better FPS");
}

void frameStep(float dt)
{
    ImGui::Begin("TERRN BATCHER [ALPHA]", /*open:*/true, ImGuiWindowFlags_AlwaysAutoResize);
    tbUI.draw();
    ImGui::End();    
}

class TerrnBatcherUI // Based on Inspector UI, can pick scenenodes and prepare Schedule for the batching.
{
    // dictionary<string, array<uint>> tbuiSelection; ~ Used by the Pick buttons.
    // * Keeping scenenode pointers is dangerous - they aren't refcounted.
    // * We keep a dictionary "scenenode name" -> child node indices
    dictionary tbuiSelection;
    
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
        
        ImGui::Separator();
        this.drawSelectionBasket(terrnNode);
        ImGui::Separator();
        
        this.drawTreeNodeOgreSceneNodeRecursive(terrnNode);
    }
    //#endregion Draw UI - main
    
    //#region Draw UI - schedule
    void drawSelectionBasket(Ogre::SceneNode@ terrnNode)
    {
        array<string> tobjNodes = tbuiSelection.getKeys();
        ImGui::TextDisabled("Selection:");
        
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
        // just assume trivial case - single attached entity
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
            
        }
        
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
        if (ImGui::TreeNode(snode.__getUniqueName()))
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
            ImGui::PushID(snode.__getUniqueName());
            
            // Tree node closed, draw info (context-sensitive)
            ImGui::SameLine();
            if (children.length() == 0 && movables.length() == 1)
            {
                //the most usual case - a node with an entity. display the mesh name.
                ImGui::TextDisabled("-->");
                ImGui::SameLine();
                ImGui::Text(movables[0].__getUniqueName());
                ImGui::SameLine();
                if (ImGui::SmallButton("go to")) 
                {
                    game.setPersonPosition(snode.getPosition());
                }
                ImGui::SameLine();
                // +terrnBatcher - selection
                
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
                // END terrnBatcher
            }
            else 
            {
                ImGui::Text("("+children.length()+" children, "+movables.length()+" movables)");
            }
            
            ImGui::PopID(); //snode.__getUniqueName()
        }
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
}
