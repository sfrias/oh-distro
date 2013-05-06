classdef WalkingPDController < DrakeSystem
  % outputs a desired q_ddot (including floating dofs)
  properties
    nq;
    Kp;
    Kd;
    dt;
    controller_data; % pointer to shared data handle containing qtraj
    ikoptions;
    rfoot_body;
    lfoot_body;
    robot;
  end
  
  methods
    function obj = WalkingPDController(r,controller_data,options)
      typecheck(r,'Atlas');
      typecheck(controller_data,'SharedDataHandle');
            
      input_frame = r.getStateFrame;
      coords = AtlasCoordinates(r);
      obj = obj@DrakeSystem(0,0,input_frame.dim,coords.dim,true,true);
      obj = setInputFrame(obj,input_frame);
      obj = setOutputFrame(obj,coords);

      obj.controller_data = controller_data;
      obj.nq = getNumDOF(r);

      if nargin<3
        options = struct();
      end
      
      if isfield(options,'Kp')
        typecheck(options.Kp,'double');
        sizecheck(options.Kp,[obj.nq obj.nq]);
        obj.Kp = options.Kp;
      else
        obj.Kp = 170.0*eye(obj.nq);
        %obj.Kp(1:2,1:2) = zeros(2); % ignore x,y
        %obj.Kp(19:20,19:20) = 75*eye(2); % make left/right ankle joints softer
        %obj.Kp(31:32,31:32) = 75*eye(2);
      end        
        
      if isfield(options,'Kd')
        typecheck(options.Kd,'double');
        sizecheck(options.Kd,[obj.nq obj.nq]);
        obj.Kd = options.Kd;
      else
        obj.Kd = 19.0*eye(obj.nq);
        %obj.Kd(1:2,1:2) = zeros(2); % ignore x,y
        %obj.Kd(19:20,19:20) = 10*eye(2); % make left/right ankle joints softer
        %obj.Kd(31:32,31:32) = 10*eye(2);
      end        
        
      if isfield(options,'dt')
        typecheck(options.dt,'double');
        sizecheck(options.dt,[1 1]);
        obj.dt = options.dt;
      else
        obj.dt = 0.005;
      end
      
      if isfield(options,'q_nom')
        typecheck(options.q_nom,'double');
        sizecheck(options.q_nom,[obj.nq 1]);
        q_nom = options.q_nom;
      else
        d = load('data/atlas_fp.mat');
        q_nom = d.xstar(1:obj.nq);
      end
      
      % setup IK parameters
      cost = Point(r.getStateFrame,1);
      cost.base_x = 0;
      cost.base_y = 0;
      cost.base_z = 0;
      cost.base_roll = 1000;
      cost.base_pitch = 1000;
      cost.base_yaw = 0;
      cost.back_lbz = 10;
      cost.back_mby = 100;
      cost.back_ubx = 100;

      cost = double(cost);
      obj.ikoptions = struct();
      obj.ikoptions.Q = diag(cost(1:obj.nq));
      obj.ikoptions.q_nom = q_nom;

      obj = setSampleTime(obj,[obj.dt;0]); % sets controller update rate

      obj.rfoot_body = r.findLink(r.r_foot_name);
      obj.lfoot_body = r.findLink(r.l_foot_name);
      obj.robot = r;
      
    end
   
  	function y=output(obj,t,~,x)
      q = x(1:obj.nq);
      qd = x(obj.nq+1:end);

      cdata = obj.controller_data.getData();
      
      q_des = approximateIK(obj.robot,q,0,[cdata.comtraj.eval(t);nan], ...
        obj.rfoot_body,[0;0;0],cdata.rfoottraj.eval(t), ...
        obj.lfoot_body,[0;0;0],cdata.lfoottraj.eval(t),obj.ikoptions);

      err_q = q_des - q;
      nrmerr = norm(err_q,1);
      if nrmerr > 2
        err_q = err_q * (2/nrmerr);
      end
      y = obj.Kp*err_q - obj.Kd*qd;
    end
  end
  
end
