classdef SupportState
  %SUPPORTSTATE defines a set of supporting bodies and contacts for a rigid
  %  body manipulator
  
  properties (SetAccess=protected)
    bodies; % array of supporting body indices
    contact_pts; % cell array of supporting contact point indices
    contact_pts_struct; % n x 1 structure array, where n is the number
                        % of bodies with points that can collide with
                        % terrain. Each element has the following fields
                        %   * idx - Index of a body in the
                        %           RigidBodyManipulator
                        %   * pts - 3 x k array containing points on the
                        %            body specified by idx that can
                        %            collide with non-flat terrain.

    num_contact_pts;  % convenience array containing the desired number of 
                      %             contact points for each support body
    contact_surfaces; % int IDs either: 0 (terrain), -1 (any body in bullet collision world)
                      %             or (1:num_bodies) collision object ID
  end
  
  methods
    function obj = SupportState(r,bodies,contact_pts,contact_surfaces)
      typecheck(r,'Biped');
      typecheck(bodies,'double');
      obj.bodies = bodies(bodies~=0);
      
      obj.num_contact_pts=zeros(length(obj.bodies),1);
      if nargin>2
        typecheck(contact_pts,'cell');
        sizecheck(contact_pts,length(obj.bodies));
        for i=1:length(obj.bodies)
          % verify that contact point indices are valid
          terrain_contact_point_struct = getTerrainContactPoints(r,obj.bodies(i));
          if any(contact_pts{i}>size(terrain_contact_point_struct.pts,2))
            error('SupportState: contact_pts indices exceed number of contact points');
          end
          obj.contact_pts_struct(i).idx = obj.bodies(i);
          obj.contact_pts_struct(i).pts = ...
            terrain_contact_point_struct.pts(:,contact_pts{i});
          obj.num_contact_pts(i)=length(contact_pts{i});
        end
        obj.contact_pts = contact_pts;
      else
        % use all points on body
        obj.contact_pts = cell(1,length(obj.bodies));
        for i=1:length(obj.bodies)
          obj.contact_pts_struct(i) = getTerrainContactPoints(r,obj.bodies(i));
          obj.contact_pts{i} = 1:size(getBodyContacts(r,obj.bodies(i)),2);
          obj.num_contact_pts(i)=length(obj.contact_pts{i});
        end
      end
      
      if nargin>3
        obj = setContactSurfaces(obj,contact_surfaces);
      else
        obj.contact_surfaces = zeros(length(obj.bodies),1);
      end
    end
    
    function obj = setContactSurfaces(obj,contact_surfaces)
      typecheck(contact_surfaces,'double');
      sizecheck(contact_surfaces,length(obj.bodies));
      obj.contact_surfaces = contact_surfaces;
    end
    
    function pts = contactPositions(obj,r,kinsol)
      pts=[];
      for i=1:length(obj.bodies)
        pts = [pts,forwardKin(r,kinsol,obj.contact_pts_struct(i).idx, ...
                              obj.contact_pts_struct(i).pts)];
      end
    end
    
    function obj = removeBody(obj,index_into_bodies_not_body_idx)
      obj.bodies(index_into_bodies_not_body_idx)=[];
      obj.contact_pts(index_into_bodies_not_body_idx)=[]; % note: this is correct, even though it's a cell array
      obj.num_contact_pts(index_into_bodies_not_body_idx)=[];
      obj.contact_surfaces(index_into_bodies_not_body_idx)=[];
      obj.contact_pts_struct(index_into_bodies_not_body_idx) = [];
    end
    
    function obj = removeBodyIdx(obj,body_idx)
      ind = find(obj.bodies==body_idx);
      if ~isempty(idx)
        obj = removeBody(obj,ind);
      end
    end
  end
  
end

