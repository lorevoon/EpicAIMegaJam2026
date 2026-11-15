import unreal as u
B='/Game/DirectorsCut/Animations/Act1Revision14';e=u.EditorAssetLibrary;m=u.MaterialEditingLibrary;mat=u.load_asset(B+'/M_RomeoFriendlyFace');node=u.load_object(None,mat.get_path_name()+':MaterialExpressionCustom_0');sources=dict(zip(m.get_material_expression_input_names(node),m.get_inputs_for_material_expression(mat,node)));sample=sources['Mask'];uv=sources['UV'];param=m.create_material_expression(mat,u.MaterialExpressionScalarParameter,-500,1800);param.set_editor_property('parameter_name','FaceSmile');shape=m.create_material_expression(mat,u.MaterialExpressionCustom,-200,1800);shape.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT4);inputs=[]
for name in ['Mask','UV','Smile']:
 x=u.CustomInput();x.set_editor_property('input_name',name);inputs.append(x)
shape.set_editor_property('inputs',inputs);shape.set_editor_property('code','''if (abs(ddx(UV.x))+abs(ddy(UV.x))+abs(ddx(UV.y))+abs(ddy(UV.y)) < .00001) return Mask;
if (Smile <= .5 || UV.x < .531 || UV.x > .599 || UV.y < .585 || UV.y > .634) return Mask;
float x=saturate((UV.x-.532208)/.064847);
float y=(UV.y-(.599069-.012492*x))/.031336;
float top=.44-.28*x*x;
float bottom=top+.10+.18*(1-x*x);
float alpha=smoothstep(top-.025,top+.025,y)*(1-smoothstep(bottom-.025,bottom+.025,y));
float edge=smoothstep(top+.035,top+.065,y)*(1-smoothstep(bottom-.065,bottom-.035,y));
return float4(lerp(.5,.12,edge),0,0,alpha);''');m.connect_material_expressions(sample,'RGBA',shape,'Mask');m.connect_material_expressions(uv,'',shape,'UV');m.connect_material_expressions(param,'',shape,'Smile');m.connect_material_expressions(shape,'',node,'Mask');alpha=m.create_material_expression(mat,u.MaterialExpressionComponentMask,0,1800);alpha.set_editor_property('r',False);alpha.set_editor_property('g',False);alpha.set_editor_property('b',False);alpha.set_editor_property('a',True);m.connect_material_expressions(shape,'',alpha,'');m.connect_material_property(alpha,'',u.MaterialProperty.MP_OPACITY_MASK);m.recompile_material(mat);assert e.save_loaded_asset(mat,False);print('ROMEO_UPTURNED_SMILE_SAVED')
